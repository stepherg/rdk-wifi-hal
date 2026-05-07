/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Docker radio simulation — populate HAL radio/interface tables in software.
 *
 * Replaces the nl80211 GET_WIPHY / GET_INTERFACE discovery path when
 * DOCKER_SIM_PORT is defined.  Three synthetic radios are created:
 *   Radio 0 (phy_index 0) — 2.4 GHz
 *   Radio 1 (phy_index 1) — 5 GHz
 *   Radio 2 (phy_index 2) — 6 GHz
 *
 * Interface entries are taken from the static interface_index_map[] entries
 * that are compiled in for DOCKER_SIM_PORT (see wifi_hal_nl80211_utils.c).
 */

#include <string.h>
#include <stdlib.h>
#include <net/if.h>       /* if_nametoindex */
#include <linux/nl80211.h>

#include "wifi_hal_priv.h"
#include "wifi_hal.h"
#include "collection.h"
#include "wifi_hal_radio_sim.h"

/* Channel arrays and their counts defined in wifi_hal_nl80211_utils.c */
extern const unsigned int wifi_2g_channels[];
extern const unsigned int wifi_2g_channels_count;
extern const unsigned int wifi_5g_channels[];
extern const unsigned int wifi_5g_channels_count;
extern const unsigned int wifi_6g_channels[];
extern const unsigned int wifi_6g_channels_count;

/* Declared in wifi_hal_nl80211_utils.c */
extern unsigned int get_sizeof_interfaces_index_map(void);
extern void get_wifi_interface_info_map(wifi_interface_name_idex_map_t *interface_map);
extern void get_radio_interface_info_map(radio_interface_mapping_t *radio_interface_map);
extern int  set_interface_properties(unsigned int phy_index, wifi_interface_info_t *interface);
extern int  get_mac_address(char *intf_name, mac_address_t mac);

/* Per-radio static parameters (index = rdk_radio_index / phy_index order) */
static const struct {
    enum nl80211_band   nl_band;
    enum hostapd_hw_mode hw_mode;
} sim_radio_band[3] = {
    { NL80211_BAND_2GHZ, HOSTAPD_MODE_IEEE80211G },
    { NL80211_BAND_5GHZ, HOSTAPD_MODE_IEEE80211A },
    { NL80211_BAND_6GHZ, HOSTAPD_MODE_IEEE80211A },
};

/*
 * Calculate frequency (MHz) for a channel number given the nl80211_band.
 * These are the standard formulae used by cfg80211.
 */
static int sim_chan_to_freq(enum nl80211_band band, unsigned int chan)
{
    if (band == NL80211_BAND_2GHZ) {
        if (chan == 14) return 2484;
        return 2407 + (int)chan * 5;
    }
    if (band == NL80211_BAND_5GHZ) {
        return 5000 + (int)chan * 5;
    }
    /* 6 GHz: 5950 + channel*5  (first usable channel = 1 → 5955 MHz) */
    return 5950 + (int)chan * 5;
}

/*
 * Fill the hw_modes entry and channel_data array for one radio/band pair.
 */
static void sim_populate_band(wifi_radio_info_t *radio, enum nl80211_band band,
    const unsigned int *chans, unsigned int n_chans)
{
    struct hostapd_hw_modes *mode = &radio->hw_modes[band];
    unsigned int i;

    if (n_chans > MAX_CHANNELS) {
        n_chans = MAX_CHANNELS;
    }

    mode->mode        = sim_radio_band[radio->rdk_radio_index].hw_mode;
    mode->num_channels = (int)n_chans;
    mode->channels    = radio->channel_data[band]; /* point into inline array */

    for (i = 0; i < n_chans; i++) {
        struct hostapd_channel_data *ch = &radio->channel_data[band][i];
        memset(ch, 0, sizeof(*ch));
        ch->chan      = (short)chans[i];
        ch->freq      = sim_chan_to_freq(band, chans[i]);
        ch->flag      = 0;
        ch->allowed_bw = ~0u; /* allow all widths */
    }
}

int populate_simulated_radios(void)
{
    unsigned int n_iface;
    unsigned int radio_map_size;
    radio_interface_mapping_t *radio_map;
    int r;

    radio_map_size = get_sizeof_radio_interfaces_map();
    if (radio_map_size < 3) {
        wifi_hal_error_print("%s:%d: invalid radio map size %u\n", __func__, __LINE__,
            radio_map_size);
        return -1;
    }

    radio_map = calloc(radio_map_size, sizeof(*radio_map));
    if (radio_map == NULL) {
        wifi_hal_error_print("%s:%d: calloc failed for radio_map size %u\n", __func__, __LINE__,
            radio_map_size);
        return -1;
    }

    get_radio_interface_info_map(radio_map);

    n_iface = get_sizeof_interfaces_index_map();
    wifi_interface_name_idex_map_t iface_map[n_iface]; /* VLA — bounded by map size */
    get_wifi_interface_info_map(iface_map);

    for (r = 0; r < 3; r++) {
        wifi_radio_info_t *radio = &g_wifi_hal.radio_info[r];
        unsigned int phy_index       = radio_map[r].phy_index;
        unsigned int rdk_radio_index = radio_map[r].radio_index;
        enum nl80211_band band       = sim_radio_band[r].nl_band;
        unsigned int j;

        /* Basic radio identity */
        strncpy(radio->name, radio_map[r].interface_name,
                sizeof(radio->name) - 1);
        radio->index          = phy_index;
        radio->rdk_radio_index = rdk_radio_index;
        radio->radio_presence = true;

        /* Advertise AP capability */
        radio->driver_data.capa.flags |= WPA_DRIVER_FLAGS_AP;

        /* Populate channel data for the primary band of this radio */
        switch (band) {
        case NL80211_BAND_2GHZ:
            sim_populate_band(radio, band, wifi_2g_channels, wifi_2g_channels_count);
            radio->capab.band[0] = WIFI_FREQUENCY_2_4_BAND;
            radio->oper_param.band = WIFI_FREQUENCY_2_4_BAND;
            break;
        case NL80211_BAND_5GHZ:
            sim_populate_band(radio, band, wifi_5g_channels, wifi_5g_channels_count);
            radio->capab.band[0] = WIFI_FREQUENCY_5_BAND;
            radio->oper_param.band = WIFI_FREQUENCY_5_BAND;
            break;
        case NL80211_BAND_6GHZ:
            sim_populate_band(radio, band, wifi_6g_channels, wifi_6g_channels_count);
            radio->capab.band[0] = WIFI_FREQUENCY_6_BAND;
            radio->oper_param.band = WIFI_FREQUENCY_6_BAND;
            break;
        default:
            break;
        }

        /* Create the per-radio interface hash map */
        radio->interface_map = hash_map_create();
        if (radio->interface_map == NULL) {
            wifi_hal_error_print("%s:%d: hash_map_create failed for radio %d\n",
                __func__, __LINE__, r);
            free(radio_map);
            return -1;
        }

        /* Populate interfaces from the static interface_index_map */
        for (j = 0; j < n_iface; j++) {
            wifi_interface_info_t *iface;

            if (iface_map[j].phy_index != phy_index) {
                continue;
            }

            iface = calloc(1, sizeof(wifi_interface_info_t));
            if (iface == NULL) {
                wifi_hal_error_print("%s:%d: calloc failed for interface %s\n",
                    __func__, __LINE__, iface_map[j].interface_name);
                free(radio_map);
                return -1;
            }

            strncpy(iface->name, iface_map[j].interface_name,
                    sizeof(iface->name) - 1);
            iface->phy_index      = phy_index;
            iface->rdk_radio_index = rdk_radio_index;

            /* Get kernel ifindex — dummy interface may or may not exist yet */
            iface->index = if_nametoindex(iface->name);

            /* Get MAC address if the interface exists in the kernel */
            if (iface->index != 0) {
                get_mac_address(iface->name, iface->mac);
            }

            /* Populate vap_info fields from static map */
            if (set_interface_properties(phy_index, iface) != 0) {
                wifi_hal_info_print(
                    "%s:%d: set_interface_properties failed for %s phy_index=%u"
                    " — interface skipped\n",
                    __func__, __LINE__, iface->name, phy_index);
                free(iface);
                continue;
            }

            hash_map_put(radio->interface_map, iface->name, iface);
        }

        /* maxNumberVAPs = number of VAP interfaces we found in the map */
        radio->capab.maxNumberVAPs =
            (unsigned int)hash_map_count(radio->interface_map);
        if (radio->capab.maxNumberVAPs == 0) {
            radio->capab.maxNumberVAPs = 1;
        }

        g_wifi_hal.num_radios++;
    }

    free(radio_map);

    wifi_hal_info_print("%s:%d: Populated %d simulated radios\n",
        __func__, __LINE__, g_wifi_hal.num_radios);
    return 0;
}
