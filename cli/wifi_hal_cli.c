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
 * wifi_hal_cli — Interactive command-line tool for exercising rdk-wifi-hal APIs.
 *
 * Usage:
 *   wifi_hal_cli <command> [arguments ...]
 *   wifi_hal_cli help [command]
 *   wifi_hal_cli list [filter]
 *
 * Build:
 *   See accompanying Makefile.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "wifi_hal.h"

/* ------------------------------------------------------------------ */
/*  Declarations for internal wifi_hal_* functions (librdk_wifihal.so) */
/*  These are the same APIs used by OneWifi — see wifi_hal_priv.h.    */
/* ------------------------------------------------------------------ */
extern INT  wifi_hal_init(void);
extern INT  wifi_hal_pre_init(void);
extern INT  wifi_hal_getHalCapability(wifi_hal_capability_t *hal);
extern INT  wifi_hal_getRadioVapInfoMap(wifi_radio_index_t index, wifi_vap_info_map_t *map);
extern INT  wifi_hal_createVAP(wifi_radio_index_t index, wifi_vap_info_map_t *map);
extern INT  wifi_hal_setRadioOperatingParameters(wifi_radio_index_t index, wifi_radio_operationParam_t *operationParam);
extern INT  wifi_hal_getRadioTransmitPower(INT radioIndex, ULONG *tx_power);
extern INT  wifi_hal_setRadioTransmitPower(wifi_radio_index_t radioIndex, UINT txpower);
extern INT  wifi_hal_getRadioTemperature(wifi_radio_index_t radioIndex, wifi_radioTemperature_t *output_struct);
extern INT  wifi_hal_get_RegDomain(wifi_radio_index_t radioIndex, UINT *reg_domain);
extern INT  wifi_hal_set_acs_keep_out_chans(wifi_radio_operationParam_t *wifi_radio_oper_param, int radioIndex);
extern void wifi_hal_set_mgt_frame_rate_limit(bool enable, int rate_limit, int window_size, int protocol, int radio_mask);
extern INT  wifi_hal_kickAssociatedDevice(INT ap_index, mac_address_t mac);
extern void wifi_hal_disassoc(int vap_index, int status, uint8_t *mac);
extern INT  wifi_hal_addApAclDevice(INT apIndex, CHAR *DeviceMacAddress);
extern INT  wifi_hal_delApAclDevice(INT apIndex, CHAR *DeviceMacAddress);
extern INT  wifi_hal_delApAclDevices(INT apIndex);
extern INT  wifi_hal_getApAclDeviceNum(INT apIndex, UINT *aclCount);
extern int  wifi_hal_setApMacAddressControlMode(uint32_t apIndex, uint32_t mac_filter_mode);
extern INT  wifi_hal_setApWpsButtonPush(INT apIndex);
extern INT  wifi_hal_setApWpsCancel(INT ap_index);
extern INT  wifi_hal_setApWpsPin(INT ap_index, char *wps_pin);
extern INT  wifi_hal_connect(INT ap_index, wifi_bss_info_t *bss);
extern INT  wifi_hal_disconnect(INT ap_index);
extern INT  wifi_hal_startScan(wifi_radio_index_t index, wifi_neighborScanMode_t scan_mode, INT dwell_time, UINT num, UINT *chan_list);
extern INT  wifi_hal_startNeighborScan(INT apIndex, wifi_neighborScanMode_t scan_mode, INT dwell_time, UINT chan_num, UINT *chan_list);
extern INT  wifi_hal_getNeighboringWiFiStatus(INT radioIndex, wifi_neighbor_ap2_t **neighbor_ap_array, UINT *output_array_size);
extern INT  wifi_hal_getScanResults(wifi_radio_index_t index, wifi_channel_t *channel, wifi_bss_info_t **bss, UINT *num_bss);
extern INT  wifi_hal_findNetworks(INT ap_index, wifi_channel_t *channel, wifi_bss_info_t **bss_array, UINT *num_bss);
extern INT  wifi_hal_configNeighborReports(UINT apIndex, bool enable, bool auto_resp);
extern INT  wifi_hal_sendDataFrame(int vap_id, unsigned char *dmac, unsigned char *data_buff, int data_len, BOOL insert_llc, int protocal, int priority);
extern INT  wifi_hal_setBTMRequest(UINT apIndex, mac_address_t peerMac, wifi_BTMRequest_t *request);
/* Callback registration */
extern void wifi_hal_newApAssociatedDevice_callback_register(wifi_newApAssociatedDevice_callback func);
extern void wifi_hal_apDisassociatedDevice_callback_register(wifi_device_disassociated_callback func);
extern void wifi_hal_apDeAuthEvent_callback_register(wifi_device_deauthenticated_callback func);
extern void wifi_hal_scanResults_callback_register(wifi_scanResults_callback func);
extern INT  wifi_hal_mgmt_frame_callbacks_register(wifi_receivedMgmtFrame_callback func);

/* ------------------------------------------------------------------ */
/*  Utilities                                                          */
/* ------------------------------------------------------------------ */

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(m) (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5]

static int require_args(int argc, int need, const char *usage)
{
    if (argc < need) {
        fprintf(stderr, "Error: not enough arguments.\nUsage: %s\n", usage);
        return -1;
    }
    return 0;
}

static const char *band_str(wifi_freq_bands_t b)
{
    switch (b) {
    case WIFI_FREQUENCY_2_4_BAND: return "2.4GHz";
    case WIFI_FREQUENCY_5_BAND:   return "5GHz";
    case WIFI_FREQUENCY_5L_BAND:  return "5GHz-low";
    case WIFI_FREQUENCY_5H_BAND:  return "5GHz-high";
    case WIFI_FREQUENCY_6_BAND:   return "6GHz";
    case WIFI_FREQUENCY_60_BAND:  return "60GHz";
    default: return "unknown";
    }
}

static const char *sec_mode_str(wifi_security_modes_t m)
{
    switch (m) {
    case wifi_security_mode_none:               return "none";
    case wifi_security_mode_wpa_personal:       return "wpa-personal";
    case wifi_security_mode_wpa2_personal:      return "wpa2-personal";
    case wifi_security_mode_wpa_wpa2_personal:  return "wpa/wpa2-personal";
    case wifi_security_mode_wpa_enterprise:     return "wpa-enterprise";
    case wifi_security_mode_wpa2_enterprise:    return "wpa2-enterprise";
    case wifi_security_mode_wpa_wpa2_enterprise:return "wpa/wpa2-enterprise";
    case wifi_security_mode_wpa3_personal:      return "wpa3-personal";
    case wifi_security_mode_wpa3_transition:    return "wpa3-transition";
    case wifi_security_mode_wpa3_enterprise:    return "wpa3-enterprise";
    case wifi_security_mode_enhanced_open:      return "enhanced-open";
    case wifi_security_mode_wpa3_compatibility: return "wpa3-compatibility";
    default: return "unknown";
    }
}

static const char *bw_str(wifi_channelBandwidth_t bw)
{
    switch (bw) {
    case WIFI_CHANNELBANDWIDTH_20MHZ:     return "20MHz";
    case WIFI_CHANNELBANDWIDTH_40MHZ:     return "40MHz";
    case WIFI_CHANNELBANDWIDTH_80MHZ:     return "80MHz";
    case WIFI_CHANNELBANDWIDTH_160MHZ:    return "160MHz";
    case WIFI_CHANNELBANDWIDTH_80_80MHZ:  return "80+80MHz";
    case WIFI_CHANNELBANDWIDTH_320MHZ:    return "320MHz";
    default: return "unknown";
    }
}

static void print_variants(unsigned int v)
{
    int first = 1;
    struct { unsigned int bit; const char *name; } map[] = {
        {WIFI_80211_VARIANT_A,  "a"},
        {WIFI_80211_VARIANT_B,  "b"},
        {WIFI_80211_VARIANT_G,  "g"},
        {WIFI_80211_VARIANT_N,  "n"},
        {WIFI_80211_VARIANT_H,  "h"},
        {WIFI_80211_VARIANT_AC, "ac"},
        {WIFI_80211_VARIANT_AD, "ad"},
        {WIFI_80211_VARIANT_AX, "ax"},
        {WIFI_80211_VARIANT_BE, "be"},
    };
    for (unsigned i = 0; i < sizeof(map)/sizeof(map[0]); i++) {
        if (v & map[i].bit) {
            printf("%s%s", first ? "" : "/", map[i].name);
            first = 0;
        }
    }
    if (first) printf("none");
}

/* ------------------------------------------------------------------ */
/*  Command handler type and forward declarations                      */
/* ------------------------------------------------------------------ */

typedef int (*cmd_handler_t)(int argc, char **argv);

typedef struct {
    const char  *name;
    cmd_handler_t handler;
    const char  *category;
    const char  *synopsis;
    const char  *description;
    const char  *params;
    const char  *example;
} cli_cmd_t;

static const cli_cmd_t commands[];

/* ------------------------------------------------------------------ */
/*  Auto-initialization                                               */
/* ------------------------------------------------------------------ */
static int g_hal_initialized = 0;

static int ensure_hal_init(void)
{
    if (g_hal_initialized)
        return RETURN_OK;
    fprintf(stderr, "[auto] calling wifi_hal_init()...\n");
    INT rc = wifi_hal_init();
    if (rc == RETURN_OK) {
        g_hal_initialized = 1;
    } else {
        fprintf(stderr, "[auto] wifi_hal_init() failed: %d\n", rc);
    }
    return rc;
}

/* ================================================================== */
/*  GENERIC / INIT COMMANDS                                            */
/* ================================================================== */

static int cmd_hal_init(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_hal_init();
    if (rc == RETURN_OK) g_hal_initialized = 1;
    printf("wifi_hal_init() => %d\n", rc);
    return rc;
}

static int cmd_hal_getHalCapability(int argc, char **argv)
{
    (void)argc; (void)argv;
    wifi_hal_capability_t cap;
    memset(&cap, 0, sizeof(cap));
    INT rc = wifi_hal_getHalCapability(&cap);
    if (rc == RETURN_OK) {
        printf("HAL version      : %u.%u\n", cap.version.major, cap.version.minor);
        printf("Num radios       : %u\n", cap.wifi_prop.numRadios);
        for (unsigned i = 0; i < cap.wifi_prop.numRadios && i < MAX_NUM_RADIOS; i++) {
            wifi_radio_capabilities_t *r = &cap.wifi_prop.radiocap[i];
            printf("  Radio %u:\n", i);
            printf("    Max VAPs     : %u\n", r->maxNumberVAPs);
            printf("    Band         : %s\n", band_str(r->band[0]));
        }
    }
    printf("wifi_hal_getHalCapability() => %d\n", rc);
    return rc;
}

static int cmd_factoryReset(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_factoryReset();
    printf("wifi_factoryReset() => %d\n", rc);
    return rc;
}

static int cmd_init(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_hal_init();
    if (rc == RETURN_OK) g_hal_initialized = 1;
    printf("wifi_hal_init() => %d\n", rc);
    return rc;
}

static int cmd_reset(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_reset();
    printf("wifi_reset() => %d\n", rc);
    return rc;
}

static int cmd_down(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_down();
    printf("wifi_down() => %d\n", rc);
    return rc;
}

/* ================================================================== */
/*  RADIO COMMANDS                                                     */
/* ================================================================== */

static int cmd_getRadioEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioEnable <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioEnable(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d enable: %s\n", idx, val ? "true" : "false");
    printf("wifi_getRadioEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioEnable <radioIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setRadioEnable(idx, en);
    printf("wifi_setRadioEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getRadioStatus(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioStatus <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioStatus(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d status: %s\n", idx, val ? "up" : "down");
    printf("wifi_getRadioStatus(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioIfName(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioIfName <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    CHAR name[64] = {0};
    INT rc = wifi_getRadioIfName(idx, name);
    if (rc == RETURN_OK) printf("Radio %d ifname: %s\n", idx, name);
    printf("wifi_getRadioIfName(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioDfsEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioDfsEnable <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioDfsEnable(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d DFS: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getRadioDfsEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioDfsEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioDfsEnable <radioIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setRadioDfsEnable(idx, en);
    printf("wifi_setRadioDfsEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getRadioTransmitPower(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioTransmitPower <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    ULONG val = 0;
    INT rc = wifi_hal_getRadioTransmitPower(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d TX power: %lu dBm\n", idx, val);
    printf("wifi_hal_getRadioTransmitPower(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioTransmitPower(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioTransmitPower <radioIndex> <power>")) return -1;
    INT idx = atoi(argv[0]);
    UINT pwr = (UINT)atoi(argv[1]);
    INT rc = wifi_hal_setRadioTransmitPower((wifi_radio_index_t)idx, pwr);
    printf("wifi_hal_setRadioTransmitPower(%d, %u) => %d\n", idx, pwr, rc);
    return rc;
}

static int cmd_getRadioPercentageTransmitPower(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioPercentageTransmitPower <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    ULONG val = 0;
    INT rc = wifi_getRadioPercentageTransmitPower(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d TX power: %lu%%\n", idx, val);
    printf("wifi_getRadioPercentageTransmitPower(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioMCS(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioMCS <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = 0;
    INT rc = wifi_getRadioMCS(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d MCS: %d\n", idx, val);
    printf("wifi_getRadioMCS(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioMCS(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioMCS <radioIndex> <mcs>")) return -1;
    INT idx = atoi(argv[0]);
    INT mcs = atoi(argv[1]);
    INT rc = wifi_setRadioMCS(idx, mcs);
    printf("wifi_setRadioMCS(%d, %d) => %d\n", idx, mcs, rc);
    return rc;
}

static int cmd_getRadioUpTime(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioUpTime <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    ULONG val = 0;
    INT rc = wifi_getRadioUpTime(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d uptime: %lu seconds\n", idx, val);
    printf("wifi_getRadioUpTime(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioResetCount(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioResetCount <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    ULONG val = 0;
    INT rc = wifi_getRadioResetCount(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d reset count: %lu\n", idx, val);
    printf("wifi_getRadioResetCount(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_factoryResetRadio(int argc, char **argv)
{
    if (require_args(argc, 1, "factoryResetRadio <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_factoryResetRadio(idx);
    printf("wifi_factoryResetRadio(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioTemperature(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioTemperature <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_radioTemperature_t temp = {0};
    INT rc = wifi_hal_getRadioTemperature(idx, &temp);
    if (rc == RETURN_OK) printf("Radio %d temperature: %u C\n", idx, temp.radio_Temperature);
    printf("wifi_hal_getRadioTemperature(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioAMSDUEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioAMSDUEnable <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioAMSDUEnable(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d AMSDU: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getRadioAMSDUEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioAMSDUEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioAMSDUEnable <radioIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setRadioAMSDUEnable(idx, en);
    printf("wifi_setRadioAMSDUEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_applyRadioSettings(int argc, char **argv)
{
    if (require_args(argc, 1, "applyRadioSettings <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_applyRadioSettings(idx);
    printf("wifi_applyRadioSettings(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioAutoBlockAckEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioAutoBlockAckEnable <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioAutoBlockAckEnable(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d AutoBlockAck: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getRadioAutoBlockAckEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getZeroDFSState(int argc, char **argv)
{
    if (require_args(argc, 1, "getZeroDFSState <radioIndex>")) return -1;
    UINT idx = (UINT)atoi(argv[0]);
    BOOL en = FALSE, precac = FALSE;
    INT rc = wifi_getZeroDFSState(idx, &en, &precac);
    if (rc == RETURN_OK) printf("Radio %u ZeroDFS: enable=%d precac=%d\n", idx, en, precac);
    printf("wifi_getZeroDFSState(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_setZeroDFSState(int argc, char **argv)
{
    if (require_args(argc, 3, "setZeroDFSState <radioIndex> <enable 0|1> <precac 0|1>")) return -1;
    UINT idx = (UINT)atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    BOOL precac = (BOOL)atoi(argv[2]);
    INT rc = wifi_setZeroDFSState(idx, en, precac);
    printf("wifi_setZeroDFSState(%u, %d, %d) => %d\n", idx, en, precac, rc);
    return rc;
}

static int cmd_getRadioOperatingParameters(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioOperatingParameters <radioIndex>")) return -1;
    UINT idx = (UINT)atoi(argv[0]);
    wifi_radio_operationParam_t param;
    memset(&param, 0, sizeof(param));
    INT rc = wifi_getRadioOperatingParameters(idx, &param);
    if (rc == RETURN_OK) {
        printf("Radio %u operating parameters:\n", idx);
        printf("  enable          : %s\n", param.enable ? "true" : "false");
        printf("  band            : %s\n", band_str(param.band));
        printf("  autoChannel     : %s\n", param.autoChannelEnabled ? "true" : "false");
        printf("  channel         : %u\n", param.channel);
        printf("  channelWidth    : %s\n", bw_str(param.channelWidth));
        printf("  variant         : "); print_variants(param.variant); printf("\n");
        printf("  beaconInterval  : %u\n", param.beaconInterval);
        printf("  dtimPeriod      : %u\n", param.dtimPeriod);
        printf("  transmitPower   : %u%%\n", param.transmitPower);
        printf("  operatingClass  : %u\n", param.operatingClass);
        printf("  DfsEnabled      : %s\n", param.DfsEnabled ? "true" : "false");
        printf("  stbcEnable      : %s\n", param.stbcEnable ? "true" : "false");
        printf("  ctsProtection   : %s\n", param.ctsProtection ? "true" : "false");
        printf("  obssCoex        : %s\n", param.obssCoex ? "true" : "false");
    }
    printf("wifi_getRadioOperatingParameters(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioCarrierSenseThresholdRange(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioCarrierSenseThresholdRange <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = 0;
    INT rc = wifi_getRadioCarrierSenseThresholdRange(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d CS threshold range: %d dBm\n", idx, val);
    printf("wifi_getRadioCarrierSenseThresholdRange(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioCarrierSenseThresholdInUse(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioCarrierSenseThresholdInUse <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = 0;
    INT rc = wifi_getRadioCarrierSenseThresholdInUse(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d CS threshold in use: %d dBm\n", idx, val);
    printf("wifi_getRadioCarrierSenseThresholdInUse(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioIGMPSnoopingEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioIGMPSnoopingEnable <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getRadioIGMPSnoopingEnable(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d IGMP snooping: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getRadioIGMPSnoopingEnable(%d) => %d\n", idx, rc);
    return rc;
}

/* ================================================================== */
/*  AP COMMANDS                                                        */
/* ================================================================== */

static int cmd_getApEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getApEnable <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getApEnable(idx, &val);
    if (rc == RETURN_OK) printf("AP %d enable: %s\n", idx, val ? "true" : "false");
    printf("wifi_getApEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setApEnable <apIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setApEnable(idx, en);
    printf("wifi_setApEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getApStatus(int argc, char **argv)
{
    if (require_args(argc, 1, "getApStatus <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    CHAR buf[128] = {0};
    INT rc = wifi_getApStatus(idx, buf);
    if (rc == RETURN_OK) printf("AP %d status: %s\n", idx, buf);
    printf("wifi_getApStatus(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApName(int argc, char **argv)
{
    if (require_args(argc, 1, "getApName <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    CHAR buf[128] = {0};
    INT rc = wifi_getApName(idx, buf);
    if (rc == RETURN_OK) printf("AP %d name: %s\n", idx, buf);
    printf("wifi_getApName(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApRadioIndex(int argc, char **argv)
{
    if (require_args(argc, 1, "getApRadioIndex <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT radio = -1;
    INT rc = wifi_getApRadioIndex(idx, &radio);
    if (rc == RETURN_OK) printf("AP %d radio index: %d\n", idx, radio);
    printf("wifi_getApRadioIndex(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApNumDevicesAssociated(int argc, char **argv)
{
    if (require_args(argc, 1, "getApNumDevicesAssociated <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    ULONG val = 0;
    INT rc = wifi_getApNumDevicesAssociated(idx, &val);
    if (rc == RETURN_OK) printf("AP %d associated devices: %lu\n", idx, val);
    printf("wifi_getApNumDevicesAssociated(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_kickApAssociatedDevice(int argc, char **argv)
{
    if (require_args(argc, 2, "kickApAssociatedDevice <apIndex> <mac>")) return -1;
    INT idx = atoi(argv[0]);
    mac_address_t mac;
    if (sscanf(argv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        fprintf(stderr, "Error: invalid MAC address '%s'\n", argv[1]);
        return -1;
    }
    INT rc = wifi_hal_kickAssociatedDevice(idx, mac);
    printf("wifi_hal_kickAssociatedDevice(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_getApSsidAdvertisementEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getApSsidAdvertisementEnable <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getApSsidAdvertisementEnable(idx, &val);
    if (rc == RETURN_OK) printf("AP %d SSID broadcast: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getApSsidAdvertisementEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApSsidAdvertisementEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setApSsidAdvertisementEnable <apIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setApSsidAdvertisementEnable(idx, en);
    printf("wifi_setApSsidAdvertisementEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getApWmmEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getApWmmEnable <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getApWmmEnable(idx, &val);
    if (rc == RETURN_OK) printf("AP %d WMM: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getApWmmEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApWmmEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setApWmmEnable <apIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setApWmmEnable(idx, en);
    printf("wifi_setApWmmEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getApIsolationEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "getApIsolationEnable <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL val = FALSE;
    INT rc = wifi_getApIsolationEnable(idx, &val);
    if (rc == RETURN_OK) printf("AP %d isolation: %s\n", idx, val ? "enabled" : "disabled");
    printf("wifi_getApIsolationEnable(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApIsolationEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setApIsolationEnable <apIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setApIsolationEnable(idx, en);
    printf("wifi_setApIsolationEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

static int cmd_getApMaxAssociatedDevices(int argc, char **argv)
{
    if (require_args(argc, 1, "getApMaxAssociatedDevices <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    UINT val = 0;
    INT rc = wifi_getApMaxAssociatedDevices(idx, &val);
    if (rc == RETURN_OK) printf("AP %d max associated devices: %u\n", idx, val);
    printf("wifi_getApMaxAssociatedDevices(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApMaxAssociatedDevices(int argc, char **argv)
{
    if (require_args(argc, 2, "setApMaxAssociatedDevices <apIndex> <max>")) return -1;
    INT idx = atoi(argv[0]);
    UINT val = (UINT)atoi(argv[1]);
    INT rc = wifi_setApMaxAssociatedDevices(idx, val);
    printf("wifi_setApMaxAssociatedDevices(%d, %u) => %d\n", idx, val, rc);
    return rc;
}

static int cmd_getApRetryLimit(int argc, char **argv)
{
    if (require_args(argc, 1, "getApRetryLimit <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    UINT val = 0;
    INT rc = wifi_getApRetryLimit(idx, &val);
    if (rc == RETURN_OK) printf("AP %d retry limit: %u\n", idx, val);
    printf("wifi_getApRetryLimit(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApRetryLimit(int argc, char **argv)
{
    if (require_args(argc, 2, "setApRetryLimit <apIndex> <limit>")) return -1;
    INT idx = atoi(argv[0]);
    UINT val = (UINT)atoi(argv[1]);
    INT rc = wifi_setApRetryLimit(idx, val);
    printf("wifi_setApRetryLimit(%d, %u) => %d\n", idx, val, rc);
    return rc;
}

static int cmd_getApMacAddressControlMode(int argc, char **argv)
{
    if (require_args(argc, 1, "getApMacAddressControlMode <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT mode = 0;
    INT rc = wifi_getApMacAddressControlMode(idx, &mode);
    if (rc == RETURN_OK) {
        const char *mstr = "unknown";
        switch (mode) {
        case 0: mstr = "disabled"; break;
        case 1: mstr = "whitelist"; break;
        case 2: mstr = "blacklist"; break;
        }
        printf("AP %d ACL mode: %d (%s)\n", idx, mode, mstr);
    }
    printf("wifi_getApMacAddressControlMode(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApMacAddressControlMode(int argc, char **argv)
{
    if (require_args(argc, 2, "setApMacAddressControlMode <apIndex> <0|1|2>")) return -1;
    INT idx = atoi(argv[0]);
    INT mode = atoi(argv[1]);
    INT rc = wifi_hal_setApMacAddressControlMode((uint32_t)idx, (uint32_t)mode);
    printf("wifi_hal_setApMacAddressControlMode(%d, %d) => %d\n", idx, mode, rc);
    return rc;
}

static int cmd_addApAclDevice(int argc, char **argv)
{
    if (require_args(argc, 2, "addApAclDevice <apIndex> <mac>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_addApAclDevice(idx, argv[1]);
    printf("wifi_hal_addApAclDevice(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_delApAclDevice(int argc, char **argv)
{
    if (require_args(argc, 2, "delApAclDevice <apIndex> <mac>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_delApAclDevice(idx, argv[1]);
    printf("wifi_hal_delApAclDevice(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_delApAclDevices(int argc, char **argv)
{
    if (require_args(argc, 1, "delApAclDevices <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_delApAclDevices(idx);
    printf("wifi_hal_delApAclDevices(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApAclDeviceNum(int argc, char **argv)
{
    if (require_args(argc, 1, "getApAclDeviceNum <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    UINT val = 0;
    INT rc = wifi_hal_getApAclDeviceNum(idx, &val);
    if (rc == RETURN_OK) printf("AP %d ACL entries: %u\n", idx, val);
    printf("wifi_hal_getApAclDeviceNum(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApVlanID(int argc, char **argv)
{
    if (require_args(argc, 2, "setApVlanID <apIndex> <vlanId>")) return -1;
    INT idx = atoi(argv[0]);
    INT vlan = atoi(argv[1]);
    INT rc = wifi_setApVlanID(idx, vlan);
    printf("wifi_setApVlanID(%d, %d) => %d\n", idx, vlan, rc);
    return rc;
}

static int cmd_factoryResetAP(int argc, char **argv)
{
    if (require_args(argc, 1, "factoryResetAP <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_factoryResetAP(idx);
    printf("wifi_factoryResetAP(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApBeaconRate(int argc, char **argv)
{
    if (require_args(argc, 1, "getApBeaconRate <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    char buf[64] = {0};
    INT rc = wifi_getApBeaconRate(idx, buf);
    if (rc == RETURN_OK) printf("AP %d beacon rate: %s\n", idx, buf);
    printf("wifi_getApBeaconRate(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApSecurityMFPConfig(int argc, char **argv)
{
    if (require_args(argc, 1, "getApSecurityMFPConfig <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    CHAR buf[128] = {0};
    INT rc = wifi_getApSecurityMFPConfig(idx, buf);
    if (rc == RETURN_OK) printf("AP %d MFP config: %s\n", idx, buf);
    printf("wifi_getApSecurityMFPConfig(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApSecurityMFPConfig(int argc, char **argv)
{
    if (require_args(argc, 2, "setApSecurityMFPConfig <apIndex> <Disabled|Optional|Required>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_setApSecurityMFPConfig(idx, argv[1]);
    printf("wifi_setApSecurityMFPConfig(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_getApSecurity(int argc, char **argv)
{
    if (require_args(argc, 1, "getApSecurity <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_vap_security_t sec;
    memset(&sec, 0, sizeof(sec));
    INT rc = wifi_getApSecurity(idx, &sec);
    if (rc == RETURN_OK) {
        printf("AP %d security:\n", idx);
        printf("  mode       : %s\n", sec_mode_str(sec.mode));
#if defined(WIFI_HAL_VERSION_3)
        printf("  MFP        : %d\n", sec.mfp);
#else
        printf("  MFP        : %s\n", sec.mfpConfig);
#endif
    }
    printf("wifi_getApSecurity(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioVapInfoMap(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioVapInfoMap <radioIndex>")) return -1;
    UINT idx = (UINT)atoi(argv[0]);
    wifi_vap_info_map_t map;
    memset(&map, 0, sizeof(map));
    INT rc = wifi_hal_getRadioVapInfoMap((wifi_radio_index_t)idx, &map);
    if (rc == RETURN_OK) {
        printf("Radio %u VAP info map (%u VAPs):\n", idx, map.num_vaps);
        for (unsigned i = 0; i < map.num_vaps; i++) {
            wifi_vap_info_t *v = &map.vap_array[i];
            printf("  VAP %u:\n", v->vap_index);
            printf("    name      : %s\n", v->vap_name);
            printf("    radio_idx : %u\n", v->radio_index);
            printf("    bridge    : %s\n", v->bridge_name);
            printf("    mode      : %s\n", v->vap_mode == 0 ? "AP" : "STA");
            if (v->vap_mode == 0) {
                printf("    enabled   : %s\n", v->u.bss_info.enabled ? "true" : "false");
                printf("    ssid      : %s\n", v->u.bss_info.ssid);
                printf("    bssid     : " MAC_FMT "\n", MAC_ARG(v->u.bss_info.bssid));
                printf("    security  : %s\n", sec_mode_str(v->u.bss_info.security.mode));
            }
        }
    }
    printf("wifi_hal_getRadioVapInfoMap(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_getApWpsConfiguration(int argc, char **argv)
{
    if (require_args(argc, 1, "getApWpsConfiguration <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_wps_t wps;
    memset(&wps, 0, sizeof(wps));
    INT rc = wifi_getApWpsConfiguration(idx, &wps);
    if (rc == RETURN_OK) {
        printf("AP %d WPS: enable=%d methods=%d\n", idx, wps.enable, wps.methods);
    }
    printf("wifi_getApWpsConfiguration(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApWpsButtonPush(int argc, char **argv)
{
    if (require_args(argc, 1, "setApWpsButtonPush <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_setApWpsButtonPush(idx);
    printf("wifi_hal_setApWpsButtonPush(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_cancelApWPS(int argc, char **argv)
{
    if (require_args(argc, 1, "cancelApWPS <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_setApWpsCancel(idx);
    printf("wifi_hal_setApWpsCancel(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setApWpsEnrolleePin(int argc, char **argv)
{
    if (require_args(argc, 2, "setApWpsEnrolleePin <apIndex> <pin>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_setApWpsEnrolleePin(idx, argv[1]);
    printf("wifi_setApWpsEnrolleePin(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_getApAssociatedDevice(int argc, char **argv)
{
    if (require_args(argc, 1, "getApAssociatedDevice <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    CHAR buf[4096] = {0};
    INT rc = wifi_getApAssociatedDevice(idx, buf, (INT)sizeof(buf));
    if (rc == RETURN_OK) {
        printf("AP %d associated devices:\n%s\n", idx, buf);
    }
    printf("wifi_getApAssociatedDevice(%d) => %d\n", idx, rc);
    return rc;
}

/* ================================================================== */
/*  STA COMMANDS                                                       */
/* ================================================================== */

static int cmd_disconnect(int argc, char **argv)
{
    if (require_args(argc, 1, "disconnect <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_disconnect(idx);
    printf("wifi_hal_disconnect(%d) => %d\n", idx, rc);
    return rc;
}

/* ================================================================== */
/*  TELEMETRY COMMANDS                                                 */
/* ================================================================== */

static int cmd_getRadioTrafficStats2(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioTrafficStats2 <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_radioTrafficStats2_t st;
    memset(&st, 0, sizeof(st));
    INT rc = wifi_getRadioTrafficStats2(idx, &st);
    if (rc == RETURN_OK) {
        printf("Radio %d traffic stats:\n", idx);
        printf("  BytesSent              : %lu\n", st.radio_BytesSent);
        printf("  BytesReceived          : %lu\n", st.radio_BytesReceived);
        printf("  PacketsSent            : %lu\n", st.radio_PacketsSent);
        printf("  PacketsReceived        : %lu\n", st.radio_PacketsReceived);
        printf("  ErrorsSent             : %lu\n", st.radio_ErrorsSent);
        printf("  ErrorsReceived         : %lu\n", st.radio_ErrorsReceived);
        printf("  NoiseFloor             : %d dBm\n", st.radio_NoiseFloor);
        printf("  ChannelUtilization     : %lu%%\n", st.radio_ChannelUtilization);
        printf("  ActivityFactor         : %d%%\n", st.radio_ActivityFactor);
    }
    printf("wifi_getRadioTrafficStats2(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getSSIDTrafficStats2(int argc, char **argv)
{
    if (require_args(argc, 1, "getSSIDTrafficStats2 <ssidIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_ssidTrafficStats2_t st;
    memset(&st, 0, sizeof(st));
    INT rc = wifi_getSSIDTrafficStats2(idx, &st);
    if (rc == RETURN_OK) {
        printf("SSID %d traffic stats:\n", idx);
        printf("  BytesSent              : %lu\n", st.ssid_BytesSent);
        printf("  BytesReceived          : %lu\n", st.ssid_BytesReceived);
        printf("  PacketsSent            : %lu\n", st.ssid_PacketsSent);
        printf("  PacketsReceived        : %lu\n", st.ssid_PacketsReceived);
        printf("  ErrorsSent             : %lu\n", st.ssid_ErrorsSent);
        printf("  ErrorsReceived         : %lu\n", st.ssid_ErrorsReceived);
        printf("  DiscardedSent          : %lu\n", st.ssid_DiscardedPacketsSent);
        printf("  DiscardedReceived      : %lu\n", st.ssid_DiscardedPacketsReceived);
    }
    printf("wifi_getSSIDTrafficStats2(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getWifiTrafficStats(int argc, char **argv)
{
    if (require_args(argc, 1, "getWifiTrafficStats <apIndex>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_trafficStats_t st;
    memset(&st, 0, sizeof(st));
    INT rc = wifi_getWifiTrafficStats(idx, &st);
    if (rc == RETURN_OK) {
        printf("AP %d traffic stats:\n", idx);
        printf("  ErrorsSent             : %lu\n", st.wifi_ErrorsSent);
        printf("  ErrorsReceived         : %lu\n", st.wifi_ErrorsReceived);
        printf("  UnicastSent            : %lu\n", st.wifi_UnicastPacketsSent);
        printf("  UnicastReceived        : %lu\n", st.wifi_UnicastPacketsReceived);
        printf("  MulticastSent          : %lu\n", st.wifi_MulticastPacketsSent);
        printf("  MulticastReceived      : %lu\n", st.wifi_MulticastPacketsReceived);
        printf("  BroadcastSent          : %lu\n", st.wifi_BroadcastPacketsSent);
        printf("  BroadcastReceived      : %lu\n", st.wifi_BroadcastPacketsRecevied);
    }
    printf("wifi_getWifiTrafficStats(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getRadioBandUtilization(int argc, char **argv)
{
    if (require_args(argc, 1, "getRadioBandUtilization <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = 0;
    INT rc = wifi_getRadioBandUtilization(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d band utilization: %d%%\n", idx, val);
    printf("wifi_getRadioBandUtilization(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getVAPTelemetry(int argc, char **argv)
{
    if (require_args(argc, 1, "getVAPTelemetry <apIndex>")) return -1;
    UINT idx = (UINT)atoi(argv[0]);
    wifi_VAPTelemetry_t tel;
    memset(&tel, 0, sizeof(tel));
    INT rc = wifi_getVAPTelemetry(idx, &tel);
    if (rc == RETURN_OK) {
        printf("VAP %u telemetry:\n", idx);
        printf("  txOverflow  : %u\n", tel.txOverflow);
    }
    printf("wifi_getVAPTelemetry(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_setRadioStatsEnable(int argc, char **argv)
{
    if (require_args(argc, 2, "setRadioStatsEnable <radioIndex> <0|1>")) return -1;
    INT idx = atoi(argv[0]);
    BOOL en = (BOOL)atoi(argv[1]);
    INT rc = wifi_setRadioStatsEnable(idx, en);
    printf("wifi_setRadioStatsEnable(%d, %d) => %d\n", idx, en, rc);
    return rc;
}

/* ================================================================== */
/*  BAND STEERING COMMANDS                                             */
/* ================================================================== */

static int cmd_getBandSteeringEnable(int argc, char **argv)
{
    (void)argc; (void)argv;
    BOOL val = FALSE;
    INT rc = wifi_getBandSteeringEnable(&val);
    if (rc == RETURN_OK) printf("Band steering: %s\n", val ? "enabled" : "disabled");
    printf("wifi_getBandSteeringEnable() => %d\n", rc);
    return rc;
}

static int cmd_setBandSteeringEnable(int argc, char **argv)
{
    if (require_args(argc, 1, "setBandSteeringEnable <0|1>")) return -1;
    BOOL en = (BOOL)atoi(argv[0]);
    INT rc = wifi_setBandSteeringEnable(en);
    printf("wifi_setBandSteeringEnable(%d) => %d\n", en, rc);
    return rc;
}

static int cmd_getBandSteeringRSSIThreshold(int argc, char **argv)
{
    if (require_args(argc, 1, "getBandSteeringRSSIThreshold <radioIndex>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = 0;
    INT rc = wifi_getBandSteeringRSSIThreshold(idx, &val);
    if (rc == RETURN_OK) printf("Radio %d BS RSSI threshold: %d dBm\n", idx, val);
    printf("wifi_getBandSteeringRSSIThreshold(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_setBandSteeringRSSIThreshold(int argc, char **argv)
{
    if (require_args(argc, 2, "setBandSteeringRSSIThreshold <radioIndex> <rssi>")) return -1;
    INT idx = atoi(argv[0]);
    INT val = atoi(argv[1]);
    INT rc = wifi_setBandSteeringRSSIThreshold(idx, val);
    printf("wifi_setBandSteeringRSSIThreshold(%d, %d) => %d\n", idx, val, rc);
    return rc;
}

/* ================================================================== */
/*  ADDED: OneWifi wifi_hal_* API COMMANDS                            */
/* ================================================================== */

static int cmd_hal_pre_init(int argc, char **argv)
{
    (void)argc; (void)argv;
    INT rc = wifi_hal_pre_init();
    printf("wifi_hal_pre_init() => %d\n", rc);
    return rc;
}

static int cmd_getNumRadios(int argc, char **argv)
{
    (void)argc; (void)argv;
    wifi_hal_capability_t cap;
    memset(&cap, 0, sizeof(cap));
    INT rc = wifi_hal_getHalCapability(&cap);
    if (rc == RETURN_OK)
        printf("numRadios: %u\n", cap.wifi_prop.numRadios);
    printf("wifi_hal_getHalCapability() => %d\n", rc);
    return rc;
}

static int cmd_setRadioOperatingParameters(int argc, char **argv)
{
    if (require_args(argc, 3, "setRadioOperatingParameters <radioIdx> <channel> <bw_mhz: 20|40|80|160>")) return -1;
    wifi_radio_index_t idx = (wifi_radio_index_t)atoi(argv[0]);
    wifi_radio_operationParam_t param;
    memset(&param, 0, sizeof(param));
    param.enable = TRUE;
    param.channel = (UINT)atoi(argv[1]);
    param.autoChannelEnabled = (param.channel == 0) ? TRUE : FALSE;
    switch (atoi(argv[2])) {
    case 40:  param.channelWidth = WIFI_CHANNELBANDWIDTH_40MHZ;  break;
    case 80:  param.channelWidth = WIFI_CHANNELBANDWIDTH_80MHZ;  break;
    case 160: param.channelWidth = WIFI_CHANNELBANDWIDTH_160MHZ; break;
    default:  param.channelWidth = WIFI_CHANNELBANDWIDTH_20MHZ;  break;
    }
    INT rc = wifi_hal_setRadioOperatingParameters(idx, &param);
    printf("wifi_hal_setRadioOperatingParameters(%u, ch=%u, bw=%s) => %d\n",
           idx, param.channel, bw_str(param.channelWidth), rc);
    return rc;
}

static int cmd_createVAP(int argc, char **argv)
{
    if (require_args(argc, 3, "createVAP <radioIdx> <apIdx> <ssid>")) return -1;
    wifi_radio_index_t ridx = (wifi_radio_index_t)atoi(argv[0]);
    wifi_vap_info_map_t map;
    memset(&map, 0, sizeof(map));
    map.num_vaps = 1;
    map.vap_array[0].radio_index = ridx;
    map.vap_array[0].vap_index   = (UINT)atoi(argv[1]);
    snprintf(map.vap_array[0].vap_name, sizeof(map.vap_array[0].vap_name), "wlan%d", atoi(argv[1]));
    map.vap_array[0].vap_mode    = 0; /* AP */
    snprintf(map.vap_array[0].u.bss_info.ssid, sizeof(map.vap_array[0].u.bss_info.ssid), "%s", argv[2]);
    map.vap_array[0].u.bss_info.enabled         = TRUE;
    map.vap_array[0].u.bss_info.security.mode   = wifi_security_mode_none;
    INT rc = wifi_hal_createVAP(ridx, &map);
    printf("wifi_hal_createVAP(%u, apIdx=%d, ssid=\"%s\") => %d\n",
           ridx, atoi(argv[1]), argv[2], rc);
    return rc;
}

static int cmd_get_RegDomain(int argc, char **argv)
{
    if (require_args(argc, 1, "get_RegDomain <radioIdx>")) return -1;
    wifi_radio_index_t idx = (wifi_radio_index_t)atoi(argv[0]);
    UINT domain = 0;
    INT rc = wifi_hal_get_RegDomain(idx, &domain);
    if (rc == RETURN_OK) printf("Radio %u regulatory domain: %u\n", idx, domain);
    printf("wifi_hal_get_RegDomain(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_startScan(int argc, char **argv)
{
    if (require_args(argc, 1, "startScan <radioIdx> [dwell_ms]")) return -1;
    wifi_radio_index_t idx  = (wifi_radio_index_t)atoi(argv[0]);
    INT dwell = (argc > 1) ? atoi(argv[1]) : 100;
    INT rc = wifi_hal_startScan(idx, WIFI_RADIO_SCAN_MODE_FULL, dwell, 0, NULL);
    printf("wifi_hal_startScan(%u, FULL, %d ms) => %d\n", idx, dwell, rc);
    return rc;
}

static int cmd_startNeighborScan(int argc, char **argv)
{
    if (require_args(argc, 1, "startNeighborScan <apIdx> [dwell_ms]")) return -1;
    INT ap  = atoi(argv[0]);
    INT dwell = (argc > 1) ? atoi(argv[1]) : 100;
    INT rc = wifi_hal_startNeighborScan(ap, WIFI_RADIO_SCAN_MODE_FULL, dwell, 0, NULL);
    printf("wifi_hal_startNeighborScan(ap=%d, FULL, %d ms) => %d\n", ap, dwell, rc);
    return rc;
}

static int cmd_getNeighboringWiFiStatus(int argc, char **argv)
{
    if (require_args(argc, 1, "getNeighboringWiFiStatus <radioIdx>")) return -1;
    INT idx = atoi(argv[0]);
    wifi_neighbor_ap2_t *arr = NULL;
    UINT count = 0;
    INT rc = wifi_hal_getNeighboringWiFiStatus(idx, &arr, &count);
    if (rc == RETURN_OK && arr) {
        printf("Radio %d neighboring APs: %u\n", idx, count);
        for (UINT i = 0; i < count; i++) {
            printf("  [%u] ssid=%-32s bssid=%s chan=%d rssi=%d\n",
                   i, arr[i].ap_SSID, arr[i].ap_BSSID,
                   arr[i].ap_Channel, arr[i].ap_SignalStrength);
        }
        free(arr);
    }
    printf("wifi_hal_getNeighboringWiFiStatus(%d) => %d\n", idx, rc);
    return rc;
}

static int cmd_getScanResults(int argc, char **argv)
{
    if (require_args(argc, 1, "getScanResults <radioIdx>")) return -1;
    wifi_radio_index_t idx = (wifi_radio_index_t)atoi(argv[0]);
    wifi_bss_info_t *bss = NULL;
    UINT num = 0;
    INT rc = wifi_hal_getScanResults(idx, NULL, &bss, &num);
    if (rc == RETURN_OK && bss) {
        printf("Radio %u scan results: %u BSS\n", idx, num);
        for (UINT i = 0; i < num; i++) {
            printf("  [%u] ssid=%-32s bssid=" MAC_FMT " freq=%u\n",
                   i, bss[i].ssid, MAC_ARG(bss[i].bssid), bss[i].freq);
        }
        free(bss);
    }
    printf("wifi_hal_getScanResults(%u) => %d\n", idx, rc);
    return rc;
}

static int cmd_findNetworks(int argc, char **argv)
{
    if (require_args(argc, 1, "findNetworks <apIdx>")) return -1;
    INT ap = atoi(argv[0]);
    wifi_bss_info_t *bss = NULL;
    UINT num = 0;
    INT rc = wifi_hal_findNetworks(ap, NULL, &bss, &num);
    if (rc == RETURN_OK && bss) {
        printf("AP %d found networks: %u\n", ap, num);
        for (UINT i = 0; i < num; i++) {
            printf("  [%u] ssid=%-32s bssid=" MAC_FMT " freq=%u\n",
                   i, bss[i].ssid, MAC_ARG(bss[i].bssid), bss[i].freq);
        }
        free(bss);
    }
    printf("wifi_hal_findNetworks(%d) => %d\n", ap, rc);
    return rc;
}

static int cmd_disassoc(int argc, char **argv)
{
    if (require_args(argc, 3, "disassoc <vapIdx> <status> <mac>")) return -1;
    int vap    = atoi(argv[0]);
    int status = atoi(argv[1]);
    unsigned char mac[6];
    if (sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        fprintf(stderr, "Error: invalid MAC '%s'\n", argv[2]);
        return -1;
    }
    wifi_hal_disassoc(vap, status, mac);
    printf("wifi_hal_disassoc(vap=%d, status=%d, mac=%s) called\n", vap, status, argv[2]);
    return 0;
}

static int cmd_configNeighborReports(int argc, char **argv)
{
    if (require_args(argc, 3, "configNeighborReports <apIdx> <enable 0|1> <auto_resp 0|1>")) return -1;
    UINT ap        = (UINT)atoi(argv[0]);
    bool enable    = (bool)atoi(argv[1]);
    bool auto_resp = (bool)atoi(argv[2]);
    INT rc = wifi_hal_configNeighborReports(ap, enable, auto_resp);
    printf("wifi_hal_configNeighborReports(%u, %d, %d) => %d\n", ap, enable, auto_resp, rc);
    return rc;
}

static int cmd_setApWpsPin(int argc, char **argv)
{
    if (require_args(argc, 2, "setApWpsPin <apIdx> <pin>")) return -1;
    INT idx = atoi(argv[0]);
    INT rc = wifi_hal_setApWpsPin(idx, argv[1]);
    printf("wifi_hal_setApWpsPin(%d, %s) => %d\n", idx, argv[1], rc);
    return rc;
}

static int cmd_connect(int argc, char **argv)
{
    if (require_args(argc, 3, "connect <apIdx> <ssid> <bssid>")) return -1;
    INT ap = atoi(argv[0]);
    wifi_bss_info_t bss;
    memset(&bss, 0, sizeof(bss));
    snprintf(bss.ssid, sizeof(bss.ssid), "%s", argv[1]);
    if (sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &bss.bssid[0], &bss.bssid[1], &bss.bssid[2],
               &bss.bssid[3], &bss.bssid[4], &bss.bssid[5]) != 6) {
        fprintf(stderr, "Error: invalid BSSID '%s'\n", argv[2]);
        return -1;
    }
    bss.sec_mode = wifi_security_mode_none;
    INT rc = wifi_hal_connect(ap, &bss);
    printf("wifi_hal_connect(ap=%d, ssid=\"%s\", bssid=%s) => %d\n",
           ap, argv[1], argv[2], rc);
    return rc;
}

static int cmd_set_mgt_frame_rate_limit(int argc, char **argv)
{
    if (require_args(argc, 3, "set_mgt_frame_rate_limit <enable 0|1> <rate> <window_ms>")) return -1;
    bool enable  = (bool)atoi(argv[0]);
    int  rate    = atoi(argv[1]);
    int  window  = atoi(argv[2]);
    /* proto=0 (all), radio_mask=-1 (all radios) */
    wifi_hal_set_mgt_frame_rate_limit(enable, rate, window, 0, -1);
    printf("wifi_hal_set_mgt_frame_rate_limit(enable=%d, rate=%d, window=%d, proto=0, radio_mask=-1) called\n",
           enable, rate, window);
    return 0;
}

static int cmd_sendDataFrame(int argc, char **argv)
{
    if (require_args(argc, 3, "sendDataFrame <vapId> <destMac> <hexdata>")) return -1;
    int vap = atoi(argv[0]);
    unsigned char dmac[6];
    if (sscanf(argv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &dmac[0], &dmac[1], &dmac[2], &dmac[3], &dmac[4], &dmac[5]) != 6) {
        fprintf(stderr, "Error: invalid dest MAC '%s'\n", argv[1]);
        return -1;
    }
    /* Parse hex string into bytes */
    const char *hex = argv[2];
    size_t hex_len  = strlen(hex);
    int data_len    = (int)(hex_len / 2);
    unsigned char *data = calloc(data_len + 1, 1);
    if (!data) { perror("calloc"); return -1; }
    for (int i = 0; i < data_len; i++) {
        unsigned byte;
        if (sscanf(hex + 2*i, "%02x", &byte) != 1) {
            fprintf(stderr, "Error: invalid hex data at offset %d\n", 2*i);
            free(data); return -1;
        }
        data[i] = (unsigned char)byte;
    }
    INT rc = wifi_hal_sendDataFrame(vap, dmac, data, data_len, FALSE, 0x0800, 0);
    printf("wifi_hal_sendDataFrame(vap=%d, dst=%s, len=%d) => %d\n",
           vap, argv[1], data_len, rc);
    free(data);
    return rc;
}

static int cmd_setBTMRequest(int argc, char **argv)
{
    if (require_args(argc, 2, "setBTMRequest <apIdx> <clientMac>")) return -1;
    UINT ap = (UINT)atoi(argv[0]);
    mac_address_t peer;
    if (sscanf(argv[1], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &peer[0], &peer[1], &peer[2], &peer[3], &peer[4], &peer[5]) != 6) {
        fprintf(stderr, "Error: invalid MAC '%s'\n", argv[1]);
        return -1;
    }
    wifi_BTMRequest_t req;
    memset(&req, 0, sizeof(req));
    req.token              = 1;
    req.requestMode        = 0x01; /* preferred candidate list included */
    req.disassociationImminent = 1;
    req.timer              = 200;
    req.validityInterval   = 255;
    INT rc = wifi_hal_setBTMRequest(ap, peer, &req);
    printf("wifi_hal_setBTMRequest(ap=%u, mac=%s) => %d\n", ap, argv[1], rc);
    return rc;
}

/* ================================================================== */
/*  COMMAND TABLE                                                      */
/* ================================================================== */

static const cli_cmd_t commands[] = {
    /* --- Generic / Init --- */
    {
        "down", cmd_down, "Generic",
        "down",
        "Bring down the Wi-Fi subsystem.",
        "  (no parameters)",
        "  wifi_hal_cli down"
    },
    {
        "factoryReset", cmd_factoryReset, "Generic",
        "factoryReset",
        "Restore all Wi-Fi configuration (radios + APs) to factory defaults.",
        "  (no parameters)",
        "  wifi_hal_cli factoryReset"
    },
    {
        "getHalCapability", cmd_hal_getHalCapability, "Generic",
        "getHalCapability",
        "Retrieve HAL capabilities: version, number of radios, per-radio caps (bands, max VAPs).",
        "  (no parameters)",
        "  wifi_hal_cli getHalCapability"
    },
    {
        "getNumRadios", cmd_getNumRadios, "Generic",
        "getNumRadios",
        "Get the number of physical radios reported by wifi_hal_getHalCapability.",
        "  (no parameters)",
        "  wifi_hal_cli getNumRadios"
    },
    {
        "hal_init", cmd_hal_init, "Generic",
        "hal_init",
        "Initialize the RDK Wi-Fi HAL (wifi_hal_init). Must be called before most other APIs.",
        "  (no parameters)",
        "  wifi_hal_cli hal_init"
    },
    {
        "hal_pre_init", cmd_hal_pre_init, "Generic",
        "hal_pre_init",
        "Run pre-initialization steps before hal_init (same as OneWifi startup).",
        "  (no parameters)",
        "  wifi_hal_cli hal_pre_init"
    },
    {
        "init", cmd_init, "Generic",
        "init",
        "Alias for hal_init — calls wifi_hal_init.",
        "  (no parameters)",
        "  wifi_hal_cli init"
    },
    {
        "reset", cmd_reset, "Generic",
        "reset",
        "Reset all Wi-Fi subsystem state.",
        "  (no parameters)",
        "  wifi_hal_cli reset"
    },

    /* --- Radio (wifi_hal_* APIs) --- */
    {
        "applyRadioSettings", cmd_applyRadioSettings, "Radio",
        "applyRadioSettings <radioIndex>",
        "Apply all pending radio-level parameter changes to the hardware.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli applyRadioSettings 0"
    },
    {
        "createVAP", cmd_createVAP, "Radio",
        "createVAP <radioIdx> <apIdx> <ssid>",
        "Create/configure a VAP on a radio via wifi_hal_createVAP. Creates an open AP.",
        "  radioIdx  - 0-based radio index\n"
        "  apIdx     - AP/VAP index\n"
        "  ssid      - SSID name",
        "  wifi_hal_cli createVAP 0 0 MySSID"
    },
    {
        "factoryResetRadio", cmd_factoryResetRadio, "Radio",
        "factoryResetRadio <radioIndex>",
        "Reset a specific radio to factory defaults without touching AP config.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli factoryResetRadio 0"
    },
    {
        "get_RegDomain", cmd_get_RegDomain, "Radio",
        "get_RegDomain <radioIdx>",
        "Get the regulatory domain for a radio via wifi_hal_get_RegDomain.",
        "  radioIdx  - 0-based radio index",
        "  wifi_hal_cli get_RegDomain 0"
    },
    {
        "getNeighboringWiFiStatus", cmd_getNeighboringWiFiStatus, "Radio",
        "getNeighboringWiFiStatus <radioIdx>",
        "Get neighbor APs detected on a radio via wifi_hal_getNeighboringWiFiStatus.",
        "  radioIdx  - 0-based radio index",
        "  wifi_hal_cli getNeighboringWiFiStatus 0"
    },
    {
        "getRadioAMSDUEnable", cmd_getRadioAMSDUEnable, "Radio",
        "getRadioAMSDUEnable <radioIndex>",
        "Check whether A-MSDU aggregation is enabled on a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioAMSDUEnable 0"
    },
    {
        "getRadioAutoBlockAckEnable", cmd_getRadioAutoBlockAckEnable, "Radio",
        "getRadioAutoBlockAckEnable <radioIndex>",
        "Check whether auto block ACK is enabled on a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioAutoBlockAckEnable 0"
    },
    {
        "getRadioCarrierSenseThresholdInUse", cmd_getRadioCarrierSenseThresholdInUse, "Radio",
        "getRadioCarrierSenseThresholdInUse <radioIndex>",
        "Get the carrier-sense threshold currently in use.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioCarrierSenseThresholdInUse 0"
    },
    {
        "getRadioCarrierSenseThresholdRange", cmd_getRadioCarrierSenseThresholdRange, "Radio",
        "getRadioCarrierSenseThresholdRange <radioIndex>",
        "Get the supported carrier-sense (CCA) threshold range in dBm.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioCarrierSenseThresholdRange 0"
    },
    {
        "getRadioDfsEnable", cmd_getRadioDfsEnable, "Radio",
        "getRadioDfsEnable <radioIndex>",
        "Check whether DFS (Dynamic Frequency Selection) is enabled on a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioDfsEnable 1"
    },
    {
        "getRadioEnable", cmd_getRadioEnable, "Radio",
        "getRadioEnable <radioIndex>",
        "Get the enabled/disabled state of a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioEnable 0"
    },
    {
        "getRadioIfName", cmd_getRadioIfName, "Radio",
        "getRadioIfName <radioIndex>",
        "Get the network interface name (e.g., wlan0) for a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioIfName 1"
    },
    {
        "getRadioIGMPSnoopingEnable", cmd_getRadioIGMPSnoopingEnable, "Radio",
        "getRadioIGMPSnoopingEnable <radioIndex>",
        "Check whether IGMP snooping is enabled on a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioIGMPSnoopingEnable 0"
    },
    {
        "getRadioMCS", cmd_getRadioMCS, "Radio",
        "getRadioMCS <radioIndex>",
        "Get the Modulation Coding Scheme index for a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioMCS 0"
    },
    {
        "getRadioOperatingParameters", cmd_getRadioOperatingParameters, "Radio",
        "getRadioOperatingParameters <radioIndex>",
        "Dump all radio operating parameters: channel, width, band, mode, etc.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioOperatingParameters 0"
    },
    {
        "getRadioPercentageTransmitPower", cmd_getRadioPercentageTransmitPower, "Radio",
        "getRadioPercentageTransmitPower <radioIndex>",
        "Get the current TX power expressed as a percentage.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioPercentageTransmitPower 0"
    },
    {
        "getRadioResetCount", cmd_getRadioResetCount, "Radio",
        "getRadioResetCount <radioIndex>",
        "Get the number of times a radio has been reset.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioResetCount 0"
    },
    {
        "getRadioStatus", cmd_getRadioStatus, "Radio",
        "getRadioStatus <radioIndex>",
        "Get the operational status (up/down) of a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioStatus 0"
    },
    {
        "getRadioTemperature", cmd_getRadioTemperature, "Radio",
        "getRadioTemperature <radioIndex>",
        "Get the radio chipset temperature via wifi_hal_getRadioTemperature.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioTemperature 0"
    },
    {
        "getRadioTransmitPower", cmd_getRadioTransmitPower, "Radio",
        "getRadioTransmitPower <radioIndex>",
        "Get the current TX power in dBm via wifi_hal_getRadioTransmitPower.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioTransmitPower 0"
    },
    {
        "getRadioUpTime", cmd_getRadioUpTime, "Radio",
        "getRadioUpTime <radioIndex>",
        "Get the radio uptime in seconds since it was started.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioUpTime 0"
    },
    {
        "getRadioVapInfoMap", cmd_getRadioVapInfoMap, "Radio",
        "getRadioVapInfoMap <radioIndex>",
        "Dump the full VAP information map for a radio via wifi_hal_getRadioVapInfoMap.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioVapInfoMap 0"
    },
    {
        "getScanResults", cmd_getScanResults, "Radio",
        "getScanResults <radioIdx>",
        "Retrieve cached scan results via wifi_hal_getScanResults.",
        "  radioIdx  - 0-based radio index",
        "  wifi_hal_cli getScanResults 0"
    },
    {
        "getZeroDFSState", cmd_getZeroDFSState, "Radio",
        "getZeroDFSState <radioIndex>",
        "Get the Zero-Wait DFS state and pre-CAC status for a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getZeroDFSState 1"
    },
    {
        "setRadioAMSDUEnable", cmd_setRadioAMSDUEnable, "Radio",
        "setRadioAMSDUEnable <radioIndex> <0|1>",
        "Enable or disable A-MSDU aggregation on a radio.",
        "  radioIndex  - 0-based radio index\n"
        "  enable      - 0=disable, 1=enable",
        "  wifi_hal_cli setRadioAMSDUEnable 0 1"
    },
    {
        "setRadioDfsEnable", cmd_setRadioDfsEnable, "Radio",
        "setRadioDfsEnable <radioIndex> <0|1>",
        "Enable or disable DFS on a radio.",
        "  radioIndex  - 0-based radio index\n"
        "  enable      - 0=disable, 1=enable",
        "  wifi_hal_cli setRadioDfsEnable 1 1"
    },
    {
        "setRadioEnable", cmd_setRadioEnable, "Radio",
        "setRadioEnable <radioIndex> <0|1>",
        "Enable or disable a radio.",
        "  radioIndex  - 0-based radio index\n"
        "  enable      - 0=disable, 1=enable",
        "  wifi_hal_cli setRadioEnable 0 1"
    },
    {
        "setRadioMCS", cmd_setRadioMCS, "Radio",
        "setRadioMCS <radioIndex> <mcs>",
        "Set the MCS index for a radio.",
        "  radioIndex  - 0-based radio index\n"
        "  mcs         - MCS index value",
        "  wifi_hal_cli setRadioMCS 0 7"
    },
    {
        "setRadioOperatingParameters", cmd_setRadioOperatingParameters, "Radio",
        "setRadioOperatingParameters <radioIdx> <channel> <bw_mhz>",
        "Set radio operating parameters (channel, bandwidth) via wifi_hal_setRadioOperatingParameters.\n"
        "  Use channel=0 to enable auto-channel.",
        "  radioIdx  - 0-based radio index\n"
        "  channel   - channel number (0=auto)\n"
        "  bw_mhz    - channel width: 20, 40, 80, or 160",
        "  wifi_hal_cli setRadioOperatingParameters 0 6 40"
    },
    {
        "setRadioTransmitPower", cmd_setRadioTransmitPower, "Radio",
        "setRadioTransmitPower <radioIndex> <power_pct>",
        "Set the TX power percentage via wifi_hal_setRadioTransmitPower.",
        "  radioIndex  - 0-based radio index\n"
        "  power_pct   - transmit power percentage (e.g., 75, 100)",
        "  wifi_hal_cli setRadioTransmitPower 0 100"
    },
    {
        "setZeroDFSState", cmd_setZeroDFSState, "Radio",
        "setZeroDFSState <radioIndex> <enable 0|1> <precac 0|1>",
        "Enable/disable Zero-Wait DFS. precac is relevant only in EU regulatory domain.",
        "  radioIndex  - 0-based radio index\n"
        "  enable      - 0=disable, 1=enable zero-wait DFS\n"
        "  precac      - 0=disable, 1=enable pre-CAC (EU only)",
        "  wifi_hal_cli setZeroDFSState 1 1 0"
    },
    {
        "startScan", cmd_startScan, "Radio",
        "startScan <radioIdx> [dwell_ms]",
        "Trigger a full channel scan via wifi_hal_startScan.",
        "  radioIdx  - 0-based radio index\n"
        "  dwell_ms  - dwell time per channel in ms (default 100)",
        "  wifi_hal_cli startScan 0 100"
    },

    /* --- AP (wifi_hal_* APIs) --- */
    {
        "addApAclDevice", cmd_addApAclDevice, "AP",
        "addApAclDevice <apIndex> <mac>",
        "Add a MAC to the ACL via wifi_hal_addApAclDevice.",
        "  apIndex  - VAP index\n"
        "  mac      - MAC address aa:bb:cc:dd:ee:ff",
        "  wifi_hal_cli addApAclDevice 0 aa:bb:cc:dd:ee:ff"
    },
    {
        "cancelApWPS", cmd_cancelApWPS, "AP",
        "cancelApWPS <apIndex>",
        "Cancel WPS session via wifi_hal_setApWpsCancel.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli cancelApWPS 0"
    },
    {
        "configNeighborReports", cmd_configNeighborReports, "AP",
        "configNeighborReports <apIdx> <enable 0|1> <auto_resp 0|1>",
        "Configure 802.11k neighbor report generation via wifi_hal_configNeighborReports.",
        "  apIdx      - AP/VAP index\n"
        "  enable     - 0=disable, 1=enable neighbor reports\n"
        "  auto_resp  - 0=manual, 1=auto-respond to neighbor requests",
        "  wifi_hal_cli configNeighborReports 0 1 1"
    },
    {
        "delApAclDevice", cmd_delApAclDevice, "AP",
        "delApAclDevice <apIndex> <mac>",
        "Remove a MAC from the ACL via wifi_hal_delApAclDevice.",
        "  apIndex  - VAP index\n"
        "  mac      - MAC address aa:bb:cc:dd:ee:ff",
        "  wifi_hal_cli delApAclDevice 0 aa:bb:cc:dd:ee:ff"
    },
    {
        "delApAclDevices", cmd_delApAclDevices, "AP",
        "delApAclDevices <apIndex>",
        "Clear all ACL entries via wifi_hal_delApAclDevices.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli delApAclDevices 0"
    },
    {
        "disassoc", cmd_disassoc, "AP",
        "disassoc <vapIdx> <status> <mac>",
        "Send a disassociation to a client via wifi_hal_disassoc.",
        "  vapIdx  - VAP index\n"
        "  status  - disassoc status code\n"
        "  mac     - client MAC address",
        "  wifi_hal_cli disassoc 0 1 aa:bb:cc:dd:ee:ff"
    },
    {
        "factoryResetAP", cmd_factoryResetAP, "AP",
        "factoryResetAP <apIndex>",
        "Reset a specific AP to factory defaults.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli factoryResetAP 0"
    },
    {
        "findNetworks", cmd_findNetworks, "AP",
        "findNetworks <apIdx>",
        "Scan and return visible networks from a STA VAP via wifi_hal_findNetworks.",
        "  apIdx  - STA VAP index",
        "  wifi_hal_cli findNetworks 9"
    },
    {
        "getApAclDeviceNum", cmd_getApAclDeviceNum, "AP",
        "getApAclDeviceNum <apIndex>",
        "Get ACL entry count via wifi_hal_getApAclDeviceNum.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApAclDeviceNum 0"
    },
    {
        "getApAssociatedDevice", cmd_getApAssociatedDevice, "AP",
        "getApAssociatedDevice <apIndex>",
        "List MAC addresses of all clients currently associated with an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApAssociatedDevice 0"
    },
    {
        "getApBeaconRate", cmd_getApBeaconRate, "AP",
        "getApBeaconRate <apIndex>",
        "Get the current beacon data rate string for an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApBeaconRate 0"
    },
    {
        "getApEnable", cmd_getApEnable, "AP",
        "getApEnable <apIndex>",
        "Get the enabled/disabled state of an Access Point.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApEnable 0"
    },
    {
        "getApIsolationEnable", cmd_getApIsolationEnable, "AP",
        "getApIsolationEnable <apIndex>",
        "Check whether client-to-client traffic isolation is enabled.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApIsolationEnable 0"
    },
    {
        "getApMacAddressControlMode", cmd_getApMacAddressControlMode, "AP",
        "getApMacAddressControlMode <apIndex>",
        "Get MAC ACL filter mode: 0=disabled, 1=whitelist, 2=blacklist.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApMacAddressControlMode 0"
    },
    {
        "getApMaxAssociatedDevices", cmd_getApMaxAssociatedDevices, "AP",
        "getApMaxAssociatedDevices <apIndex>",
        "Get the maximum number of client STAs allowed to associate.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApMaxAssociatedDevices 0"
    },
    {
        "getApName", cmd_getApName, "AP",
        "getApName <apIndex>",
        "Get the name / label for an AP (e.g., \"private_ssid_2g\").",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApName 0"
    },
    {
        "getApNumDevicesAssociated", cmd_getApNumDevicesAssociated, "AP",
        "getApNumDevicesAssociated <apIndex>",
        "Get the number of client stations currently associated with an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApNumDevicesAssociated 0"
    },
    {
        "getApRadioIndex", cmd_getApRadioIndex, "AP",
        "getApRadioIndex <apIndex>",
        "Get the radio index that an AP belongs to.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApRadioIndex 0"
    },
    {
        "getApRetryLimit", cmd_getApRetryLimit, "AP",
        "getApRetryLimit <apIndex>",
        "Get the frame retry limit for an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApRetryLimit 0"
    },
    {
        "getApSecurity", cmd_getApSecurity, "AP",
        "getApSecurity <apIndex>",
        "Get the security configuration (mode, MFP) for an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApSecurity 0"
    },
    {
        "getApSecurityMFPConfig", cmd_getApSecurityMFPConfig, "AP",
        "getApSecurityMFPConfig <apIndex>",
        "Get the Management Frame Protection mode (Disabled|Optional|Required).",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApSecurityMFPConfig 0"
    },
    {
        "getApSsidAdvertisementEnable", cmd_getApSsidAdvertisementEnable, "AP",
        "getApSsidAdvertisementEnable <apIndex>",
        "Check whether SSID is being broadcast in beacons/probes.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApSsidAdvertisementEnable 0"
    },
    {
        "getApStatus", cmd_getApStatus, "AP",
        "getApStatus <apIndex>",
        "Get the status string for an AP (e.g., \"Up\", \"Down\").",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApStatus 0"
    },
    {
        "getApWmmEnable", cmd_getApWmmEnable, "AP",
        "getApWmmEnable <apIndex>",
        "Check whether WMM (QoS) is enabled on an AP.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApWmmEnable 0"
    },
    {
        "getApWpsConfiguration", cmd_getApWpsConfiguration, "AP",
        "getApWpsConfiguration <apIndex>",
        "Get WPS configuration for an AP (enable state, methods).",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getApWpsConfiguration 0"
    },
    {
        "kickAssociatedDevice", cmd_kickApAssociatedDevice, "AP",
        "kickAssociatedDevice <apIdx> <mac>",
        "Kick a client STA from an AP via wifi_hal_kickAssociatedDevice.",
        "  apIdx  - VAP index\n"
        "  mac    - client MAC address (aa:bb:cc:dd:ee:ff)",
        "  wifi_hal_cli kickAssociatedDevice 0 aa:bb:cc:dd:ee:ff"
    },
    {
        "setApEnable", cmd_setApEnable, "AP",
        "setApEnable <apIndex> <0|1>",
        "Enable or disable an Access Point.",
        "  apIndex  - VAP index\n"
        "  enable   - 0=disable, 1=enable",
        "  wifi_hal_cli setApEnable 0 1"
    },
    {
        "setApIsolationEnable", cmd_setApIsolationEnable, "AP",
        "setApIsolationEnable <apIndex> <0|1>",
        "Enable or disable client isolation.",
        "  apIndex  - VAP index\n"
        "  enable   - 0=disable, 1=enable",
        "  wifi_hal_cli setApIsolationEnable 0 1"
    },
    {
        "setApMacAddressControlMode", cmd_setApMacAddressControlMode, "AP",
        "setApMacAddressControlMode <apIndex> <0|1|2>",
        "Set MAC ACL mode via wifi_hal_setApMacAddressControlMode.\n"
        "  0=disabled, 1=whitelist, 2=blacklist.",
        "  apIndex  - VAP index\n"
        "  mode     - 0/1/2",
        "  wifi_hal_cli setApMacAddressControlMode 0 2"
    },
    {
        "setApMaxAssociatedDevices", cmd_setApMaxAssociatedDevices, "AP",
        "setApMaxAssociatedDevices <apIndex> <max>",
        "Set the maximum number of allowed associated clients. 0 = unlimited.",
        "  apIndex  - VAP index\n"
        "  max      - maximum client count (0=unlimited)",
        "  wifi_hal_cli setApMaxAssociatedDevices 0 32"
    },
    {
        "setApRetryLimit", cmd_setApRetryLimit, "AP",
        "setApRetryLimit <apIndex> <limit>",
        "Set the frame retry limit.",
        "  apIndex  - VAP index\n"
        "  limit    - retry count (e.g., 7)",
        "  wifi_hal_cli setApRetryLimit 0 7"
    },
    {
        "setApSecurityMFPConfig", cmd_setApSecurityMFPConfig, "AP",
        "setApSecurityMFPConfig <apIndex> <Disabled|Optional|Required>",
        "Set the MFP mode for an AP.",
        "  apIndex  - VAP index\n"
        "  config   - Disabled, Optional, or Required",
        "  wifi_hal_cli setApSecurityMFPConfig 0 Required"
    },
    {
        "setApSsidAdvertisementEnable", cmd_setApSsidAdvertisementEnable, "AP",
        "setApSsidAdvertisementEnable <apIndex> <0|1>",
        "Enable or disable SSID broadcast.",
        "  apIndex  - VAP index\n"
        "  enable   - 0=hide SSID, 1=broadcast SSID",
        "  wifi_hal_cli setApSsidAdvertisementEnable 0 1"
    },
    {
        "setApVlanID", cmd_setApVlanID, "AP",
        "setApVlanID <apIndex> <vlanId>",
        "Set the VLAN ID for an AP.",
        "  apIndex  - VAP index\n"
        "  vlanId   - VLAN ID (integer)",
        "  wifi_hal_cli setApVlanID 0 100"
    },
    {
        "setApWmmEnable", cmd_setApWmmEnable, "AP",
        "setApWmmEnable <apIndex> <0|1>",
        "Enable or disable WMM on an AP.",
        "  apIndex  - VAP index\n"
        "  enable   - 0=disable, 1=enable",
        "  wifi_hal_cli setApWmmEnable 0 1"
    },
    {
        "setApWpsButtonPush", cmd_setApWpsButtonPush, "AP",
        "setApWpsButtonPush <apIndex>",
        "Trigger WPS PBC session via wifi_hal_setApWpsButtonPush.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli setApWpsButtonPush 0"
    },
    {
        "setApWpsEnrolleePin", cmd_setApWpsEnrolleePin, "AP",
        "setApWpsEnrolleePin <apIndex> <pin>",
        "Set a WPS enrollee PIN for PIN-based WPS configuration.",
        "  apIndex  - VAP index\n"
        "  pin      - 8-digit WPS PIN",
        "  wifi_hal_cli setApWpsEnrolleePin 0 12345670"
    },
    {
        "setApWpsPin", cmd_setApWpsPin, "AP",
        "setApWpsPin <apIndex> <pin>",
        "Set a WPS PIN for an AP via wifi_hal_setApWpsPin.",
        "  apIndex  - VAP index\n"
        "  pin      - 8-digit WPS PIN",
        "  wifi_hal_cli setApWpsPin 0 12345670"
    },
    {
        "setBTMRequest", cmd_setBTMRequest, "AP",
        "setBTMRequest <apIdx> <clientMac>",
        "Send a BSS Transition Management request to a client via wifi_hal_setBTMRequest.",
        "  apIdx      - AP/VAP index\n"
        "  clientMac  - target client MAC address",
        "  wifi_hal_cli setBTMRequest 0 aa:bb:cc:dd:ee:ff"
    },
    {
        "startNeighborScan", cmd_startNeighborScan, "AP",
        "startNeighborScan <apIdx> [dwell_ms]",
        "Trigger an off-channel neighbor scan from an AP via wifi_hal_startNeighborScan.",
        "  apIdx    - AP/VAP index\n"
        "  dwell_ms - dwell time per channel in ms (default 100)",
        "  wifi_hal_cli startNeighborScan 0 100"
    },

    /* --- STA (wifi_hal_* APIs) --- */
    {
        "connect", cmd_connect, "STA",
        "connect <apIdx> <ssid> <bssid>",
        "Connect a STA VAP to a network via wifi_hal_connect.",
        "  apIdx  - STA VAP index\n"
        "  ssid   - target SSID\n"
        "  bssid  - target BSSID (aa:bb:cc:dd:ee:ff)",
        "  wifi_hal_cli connect 9 MyNetwork aa:bb:cc:dd:ee:ff"
    },
    {
        "disconnect", cmd_disconnect, "STA",
        "disconnect <apIndex>",
        "Disconnect a STA VAP from the current BSS via wifi_hal_disconnect.",
        "  apIndex  - STA VAP index",
        "  wifi_hal_cli disconnect 9"
    },
    {
        "sendDataFrame", cmd_sendDataFrame, "STA",
        "sendDataFrame <vapId> <destMac> <hexdata>",
        "Send a raw data frame via wifi_hal_sendDataFrame.",
        "  vapId    - VAP index\n"
        "  destMac  - destination MAC address\n"
        "  hexdata  - payload as a hex string (e.g., deadbeef)",
        "  wifi_hal_cli sendDataFrame 0 aa:bb:cc:dd:ee:ff deadbeef"
    },
    {
        "set_mgt_frame_rate_limit", cmd_set_mgt_frame_rate_limit, "STA",
        "set_mgt_frame_rate_limit <enable 0|1> <rate> <window_ms>",
        "Enable/disable management frame rate limiting via wifi_hal_set_mgt_frame_rate_limit.",
        "  enable    - 0=disable, 1=enable\n"
        "  rate      - max frames per window\n"
        "  window_ms - window size in ms",
        "  wifi_hal_cli set_mgt_frame_rate_limit 1 100 1000"
    },

    /* --- Telemetry --- */
    {
        "getRadioBandUtilization", cmd_getRadioBandUtilization, "Telemetry",
        "getRadioBandUtilization <radioIndex>",
        "Get the band utilization percentage for a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioBandUtilization 0"
    },
    {
        "getRadioTrafficStats2", cmd_getRadioTrafficStats2, "Telemetry",
        "getRadioTrafficStats2 <radioIndex>",
        "Get radio-level traffic counters: bytes, packets, errors, noise floor,\n"
        "  channel utilization, activity factor, etc.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getRadioTrafficStats2 0"
    },
    {
        "getSSIDTrafficStats2", cmd_getSSIDTrafficStats2, "Telemetry",
        "getSSIDTrafficStats2 <ssidIndex>",
        "Get SSID-level traffic counters: bytes, packets, errors, discards.",
        "  ssidIndex  - SSID/VAP index",
        "  wifi_hal_cli getSSIDTrafficStats2 0"
    },
    {
        "getVAPTelemetry", cmd_getVAPTelemetry, "Telemetry",
        "getVAPTelemetry <apIndex>",
        "Get VAP-level telemetry counters (e.g., TX overflow).",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getVAPTelemetry 0"
    },
    {
        "getWifiTrafficStats", cmd_getWifiTrafficStats, "Telemetry",
        "getWifiTrafficStats <apIndex>",
        "Get per-AP traffic stats: unicast/multicast/broadcast packet counts.",
        "  apIndex  - VAP index",
        "  wifi_hal_cli getWifiTrafficStats 0"
    },
    {
        "setRadioStatsEnable", cmd_setRadioStatsEnable, "Telemetry",
        "setRadioStatsEnable <radioIndex> <0|1>",
        "Enable or disable radio statistics collection.",
        "  radioIndex  - 0-based radio index\n"
        "  enable      - 0=disable, 1=enable",
        "  wifi_hal_cli setRadioStatsEnable 0 1"
    },

    /* --- Band Steering --- */
    {
        "getBandSteeringEnable", cmd_getBandSteeringEnable, "BandSteering",
        "getBandSteeringEnable",
        "Check if band steering is globally enabled.",
        "  (no parameters)",
        "  wifi_hal_cli getBandSteeringEnable"
    },
    {
        "getBandSteeringRSSIThreshold", cmd_getBandSteeringRSSIThreshold, "BandSteering",
        "getBandSteeringRSSIThreshold <radioIndex>",
        "Get the RSSI threshold for band steering decisions on a radio.",
        "  radioIndex  - 0-based radio index",
        "  wifi_hal_cli getBandSteeringRSSIThreshold 0"
    },
    {
        "setBandSteeringEnable", cmd_setBandSteeringEnable, "BandSteering",
        "setBandSteeringEnable <0|1>",
        "Globally enable or disable band steering.",
        "  enable  - 0=disable, 1=enable",
        "  wifi_hal_cli setBandSteeringEnable 1"
    },
    {
        "setBandSteeringRSSIThreshold", cmd_setBandSteeringRSSIThreshold, "BandSteering",
        "setBandSteeringRSSIThreshold <radioIndex> <rssi>",
        "Set the RSSI threshold (dBm) for band steering on a radio.",
        "  radioIndex  - 0-based radio index\n"
        "  rssi        - threshold in dBm (e.g., -70)",
        "  wifi_hal_cli setBandSteeringRSSIThreshold 0 -70"
    },

    {NULL, NULL, NULL, NULL, NULL, NULL, NULL}  /* sentinel */
};



/* ================================================================== */
/*  HELP SYSTEM                                                        */
/* ================================================================== */

static void print_banner(void)
{
    printf("wifi_hal_cli — RDK Wi-Fi HAL Command-Line Interface\n\n");
}

static void print_usage(void)
{
    print_banner();
    printf("Usage:\n");
    printf("  wifi_hal_cli <command> [args ...]\n");
    printf("  wifi_hal_cli help [command]        Show detailed help for a command\n");
    printf("  wifi_hal_cli list [filter]          List available commands (optionally filtered)\n");
    printf("\nRun 'wifi_hal_cli list' to see all available commands.\n");
    printf("Run 'wifi_hal_cli help <command>' for detailed usage of a specific command.\n");
}

static void print_command_help(const cli_cmd_t *cmd)
{
    printf("Command: %s\n", cmd->name);
    printf("Category: %s\n\n", cmd->category);
    printf("Synopsis:\n  wifi_hal_cli %s\n\n", cmd->synopsis);
    printf("Description:\n  %s\n\n", cmd->description);
    printf("Parameters:\n%s\n\n", cmd->params);
    printf("Example:\n%s\n", cmd->example);
}

static int cmd_help(int argc, char **argv)
{
    if (argc < 1) {
        print_usage();
        return 0;
    }

    const char *name = argv[0];
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcasecmp(commands[i].name, name) == 0) {
            print_command_help(&commands[i]);
            return 0;
        }
    }
    fprintf(stderr, "Unknown command: %s\n", name);
    fprintf(stderr, "Run 'wifi_hal_cli list' to see all available commands.\n");
    return -1;
}

static int cmd_list(int argc, char **argv)
{
    const char *filter = (argc > 0) ? argv[0] : NULL;
    const char *last_cat = "";

    print_banner();
    int total = 0;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (filter != NULL) {
            /* case-insensitive substring match on name or category */
            char lname[128], lcat[64], lfilt[128];
            snprintf(lname, sizeof(lname), "%s", commands[i].name);
            snprintf(lcat, sizeof(lcat), "%s", commands[i].category);
            snprintf(lfilt, sizeof(lfilt), "%s", filter);
            for (char *p = lname; *p; p++) *p = (char)tolower(*p);
            for (char *p = lcat; *p; p++) *p = (char)tolower(*p);
            for (char *p = lfilt; *p; p++) *p = (char)tolower(*p);
            if (strstr(lname, lfilt) == NULL && strstr(lcat, lfilt) == NULL)
                continue;
        }

        if (strcmp(commands[i].category, last_cat) != 0) {
            if (total > 0) printf("\n");
            printf("[%s]\n", commands[i].category);
            last_cat = commands[i].category;
        }
        printf("  %-40s %s\n", commands[i].synopsis, commands[i].description);
        total++;
    }

    if (total == 0 && filter)
        printf("No commands matching '%s'.\n", filter);
    else
        printf("\n%d commands available. Use 'wifi_hal_cli help <cmd>' for details.\n", total);

    return 0;
}

/* ================================================================== */
/*  MAIN                                                               */
/* ================================================================== */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *cmd_name = argv[1];
    int cmd_argc = argc - 2;
    char **cmd_argv = &argv[2];

    /* Built-in meta-commands */
    if (strcasecmp(cmd_name, "help") == 0 || strcmp(cmd_name, "-h") == 0 || strcmp(cmd_name, "--help") == 0)
        return cmd_help(cmd_argc, cmd_argv);

    if (strcasecmp(cmd_name, "list") == 0 || strcasecmp(cmd_name, "ls") == 0)
        return cmd_list(cmd_argc, cmd_argv);

    /* Dispatch to command table */
    /* Skip auto-init for the init commands themselves and meta commands */
    static const char * const no_autoinit[] = {
        "hal_pre_init", "hal_init", "init", "factoryReset", "reset", "down", NULL
    };
    int skip_init = 0;
    for (int k = 0; no_autoinit[k]; k++) {
        if (strcasecmp(cmd_name, no_autoinit[k]) == 0) { skip_init = 1; break; }
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcasecmp(commands[i].name, cmd_name) == 0) {
            if (!skip_init && ensure_hal_init() != RETURN_OK)
                return 1;
            return commands[i].handler(cmd_argc, cmd_argv);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", cmd_name);
    fprintf(stderr, "Run 'wifi_hal_cli list' for available commands, or 'wifi_hal_cli help <cmd>' for help.\n");
    return 1;
}
