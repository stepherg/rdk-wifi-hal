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
 * Docker Simulation Platform
 *
 * HAL-only radio simulation for environments where mac80211_hwsim is
 * unavailable (e.g., macOS Docker Desktop). Provides three synthetic radios
 * (2.4 GHz, 5 GHz, 6 GHz) backed by kernel dummy network interfaces.
 *
 * No nl80211 phy discovery is performed. All radio/interface data is
 * populated by populate_simulated_radios() in wifi_hal_radio_sim.c.
 */

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "wifi_hal_priv.h"
#include "wifi_hal.h"

int platform_pre_init(void)
{
    wifi_hal_dbg_print("%s:%d\n", __func__, __LINE__);
    return 0;
}

int platform_post_init(wifi_vap_info_map_t *vap_map)
{
    (void)vap_map;
    wifi_hal_dbg_print("%s:%d\n", __func__, __LINE__);
    return 0;
}

int platform_set_radio(wifi_radio_index_t index, wifi_radio_operationParam_t *operationParam)
{
    (void)index;
    (void)operationParam;
    return 0;
}

int platform_set_radio_pre_init(wifi_radio_index_t index, wifi_radio_operationParam_t *operationParam)
{
    (void)index;
    (void)operationParam;
    return 0;
}

int platform_pre_create_vap(wifi_radio_index_t index, wifi_vap_info_map_t *map)
{
    (void)index;
    (void)map;
    return 0;
}

int platform_create_vap(wifi_radio_index_t index, wifi_vap_info_map_t *map)
{
    (void)index;
    (void)map;
    return 0;
}

int platform_get_ssid_default(char *ssid, int vap_index)
{
    (void)vap_index;
    snprintf(ssid, 32, "docker-sim");
    return 0;
}

int platform_get_keypassphrase_default(char *password, int vap_index)
{
    (void)vap_index;
    snprintf(password, 64, "12345678");
    return 0;
}

int platform_get_radius_key_default(char *radius_key)
{
    snprintf(radius_key, 64, "radius_key");
    return 0;
}

int platform_get_wps_pin_default(char *pin)
{
    snprintf(pin, 64, "12345670");
    return 0;
}

int platform_get_country_code_default(char *code)
{
    snprintf(code, 4, "US");
    return 0;
}

int platform_wps_event(wifi_wps_event_t data)
{
    (void)data;
    return 0;
}

int platform_flags_init(int *flags)
{
    *flags |= (int)PLATFORM_FLAGS_STA_INACTIVITY_TIMER;
    return 0;
}

int platform_get_aid(void *priv, u16 *aid, const u8 *addr)
{
    (void)priv;
    (void)aid;
    (void)addr;
    return 0;
}

int platform_free_aid(void *priv, u16 *aid)
{
    (void)priv;
    (void)aid;
    return 0;
}

int platform_sync_done(void *priv)
{
    (void)priv;
    return 0;
}

int platform_update_radio_presence(void)
{
    return 0;
}

int platform_set_txpower(void *priv, unsigned int txpower)
{
    (void)priv;
    (void)txpower;
    return 0;
}

int platform_set_offload_mode(void *priv, unsigned int offload_mode)
{
    (void)priv;
    (void)offload_mode;
    return 0;
}

int platform_get_acl_num(int vap_index, unsigned int *acl_count)
{
    (void)vap_index;
    if (acl_count != NULL) {
        *acl_count = 0;
    }
    return 0;
}

int platform_get_chanspec_list(unsigned int radioIndex, wifi_channelBandwidth_t bandwidth,
    wifi_channels_list_t channels, char *buff)
{
    (void)radioIndex;
    (void)bandwidth;
    (void)channels;
    (void)buff;
    return 0;
}

int platform_set_acs_exclusion_list(unsigned int radioIndex, char *str)
{
    (void)radioIndex;
    (void)str;
    return 0;
}

int platform_get_vendor_oui(char *vendor_oui, int vendor_oui_len)
{
    (void)vendor_oui;
    (void)vendor_oui_len;
    return 0;
}

int platform_set_neighbor_report(unsigned int apIndex, unsigned int add, mac_address_t mac)
{
    (void)apIndex;
    (void)add;
    (void)mac;
    return 0;
}

int platform_get_radio_phytemperature(wifi_radio_index_t index,
    wifi_radioTemperature_t *radioPhyTemperature)
{
    (void)index;
    if (radioPhyTemperature != NULL) {
        radioPhyTemperature->radio_Temperature = 25;
    }
    return 0;
}

int platform_set_dfs(wifi_radio_index_t index, wifi_radio_operationParam_t *operationParam)
{
    (void)index;
    (void)operationParam;
    return 0;
}

int platform_get_radio_caps(wifi_radio_index_t index)
{
    (void)index;
    return 0;
}

int platform_get_reg_domain(wifi_radio_index_t index, unsigned int *reg_domain)
{
    (void)index;
    if (reg_domain != NULL) {
        *reg_domain = 0;
    }
    return 0;
}
