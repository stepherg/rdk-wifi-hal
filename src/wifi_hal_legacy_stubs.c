/*
 * wifi_hal_legacy_stubs.c
 *
 * Docker simulation stubs for legacy wifi_* functions (non-wifi_hal_ API)
 * that OneWifi calls directly from libhal_wifi.so.
 *
 * Because librdk_wifihal.so is listed before libhal_wifi.so in OneWifi's
 * DT_NEEDED order, these definitions shadow the RPi-specific implementations
 * in libhal_wifi.so and avoid hard dependencies on /nvram/hostapd*.conf files.
 *
 * Also provides implementations for wifi_* symbols that librdk_wifihal itself
 * references as undefined (U) symbols resolved at link time.
 */

#ifdef DOCKER_SIM_PORT

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * Include only the headers whose declarations we actually match.
 * wifi_hal_deprecated.h (pulled in by wifi_hal.h) has CHAR*-based signatures
 * for wifi_addApAclDevice/delApAclDevice and a 3-arg
 * wifi_getNeighboringWiFiStatus — we use those deprecated signatures below
 * to avoid type-conflict warnings.
 */
#include "wifi_hal_rdk.h"       /* pulls in ccsp/wifi_hal.h + deprecated.h */
#include "ccsp/wifi_hal_radio.h"
#include "ccsp/wifi_hal_extender.h"
#include "ccsp/wifi_hal_telemetry.h"

/* --------------------------------------------------------------------------
 * Legacy AP management
 * -------------------------------------------------------------------------- */

/* deprecated signature: CHAR* MAC string, matching libhal_wifi.so build */
INT wifi_addApAclDevice(INT apIndex, CHAR *DeviceMacAddress)
{
    (void)apIndex; (void)DeviceMacAddress;
    return RETURN_OK;
}

INT wifi_delApAclDevice(INT apIndex, CHAR *deviceMacAddress)
{
    (void)apIndex; (void)deviceMacAddress;
    return RETURN_OK;
}

INT wifi_delApAclDevices(INT apIndex)
{
    (void)apIndex;
    return RETURN_OK;
}

INT wifi_setApMacAddressControlMode(INT apIndex, INT filterMode)
{
    (void)apIndex; (void)filterMode;
    return RETURN_OK;
}

INT wifi_getApEnable(INT apIndex, BOOL *output_bool)
{
    (void)apIndex;
    if (output_bool) *output_bool = TRUE;
    return RETURN_OK;
}

INT wifi_setApIsolationEnable(INT apIndex, BOOL enable)
{
    (void)apIndex; (void)enable;
    return RETURN_OK;
}

INT wifi_getApManagementFramePowerControl(INT apIndex, INT *output_dBm)
{
    (void)apIndex;
    if (output_dBm) *output_dBm = 0;
    return RETURN_OK;
}

INT wifi_setApManagementFramePowerControl(INT apIndex, INT dBm)
{
    (void)apIndex; (void)dBm;
    return RETURN_OK;
}

INT wifi_getApInterworkingElement(INT apIndex, wifi_InterworkingElement_t *output_struct)
{
    (void)apIndex;
    if (output_struct) memset(output_struct, 0, sizeof(*output_struct));
    return RETURN_OK;
}

INT wifi_enableCSIEngine(INT apIndex, mac_address_t sta, BOOL enable)
{
    (void)apIndex; (void)sta; (void)enable;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Action frame sending
 * -------------------------------------------------------------------------- */

INT wifi_sendActionFrame(INT apIndex, mac_address_t sta, UINT frequency,
                         UCHAR *frame, UINT len)
{
    (void)apIndex; (void)sta; (void)frequency; (void)frame; (void)len;
    return RETURN_OK;
}

INT wifi_sendActionFrameExt(INT apIndex, mac_address_t sta, UINT frequency,
                            UINT wait, UCHAR *frame, UINT len)
{
    (void)apIndex; (void)sta; (void)frequency; (void)wait; (void)frame; (void)len;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Radio parameters
 * -------------------------------------------------------------------------- */

INT wifi_getRadioChannel(INT radioIndex, ULONG *output_ulong)
{
    /* Return plausible default channels for simulated radios */
    static const ULONG default_ch[] = {6, 36, 1};
    if (output_ulong) {
        *output_ulong = (radioIndex >= 0 && radioIndex <= 2)
                        ? default_ch[radioIndex] : 0;
    }
    return RETURN_OK;
}

INT wifi_getRadioTransmitPower(INT radioIndex, ULONG *output_ulong)
{
    (void)radioIndex;
    if (output_ulong) *output_ulong = 20;
    return RETURN_OK;
}

INT wifi_setRadioDfsAtBootUpEnable(INT radioIndex, BOOL enable)
{
    (void)radioIndex; (void)enable;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Statistics / telemetry
 * -------------------------------------------------------------------------- */

INT wifi_getRadioTrafficStats2(INT radioIndex, wifi_radioTrafficStats2_t *output_struct)
{
    (void)radioIndex;
    if (output_struct) memset(output_struct, 0, sizeof(*output_struct));
    return RETURN_OK;
}

INT wifi_getRadioChannelStats(INT radioIndex,
                              wifi_channelStats_t *input_output_channelStats_array,
                              INT array_size)
{
    (void)radioIndex;
    if (input_output_channelStats_array && array_size > 0) {
        memset(input_output_channelStats_array, 0,
               (size_t)array_size * sizeof(*input_output_channelStats_array));
    }
    return RETURN_OK;
}

INT wifi_getApAssociatedDeviceDiagnosticResult3(INT apIndex,
                                                wifi_associated_dev3_t **associated_dev_array,
                                                UINT *output_array_size)
{
    (void)apIndex;
    if (associated_dev_array) *associated_dev_array = NULL;
    if (output_array_size)    *output_array_size    = 0;
    return RETURN_OK;
}

INT wifi_getApDeviceRSSI(INT ap_index, CHAR *MAC, INT *output_RSSI)
{
    (void)ap_index; (void)MAC;
    if (output_RSSI) *output_RSSI = 0;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Neighbor scan
 * -------------------------------------------------------------------------- */

/* deprecated 3-arg signature — no BOOL scan — matching libhal_wifi.so build */
INT wifi_getNeighboringWiFiStatus(INT radioIndex,
                                  wifi_neighbor_ap2_t **neighbor_ap_array,
                                  UINT *output_array_size)
{
    (void)radioIndex;
    if (neighbor_ap_array)  *neighbor_ap_array  = NULL;
    if (output_array_size)  *output_array_size  = 0;
    return RETURN_OK;
}

INT wifi_startNeighborScan(INT apIndex, wifi_neighborScanMode_t scan_mode,
                           INT dwell_time, UINT chan_num, UINT *chan_list)
{
    (void)apIndex; (void)scan_mode; (void)dwell_time; (void)chan_num; (void)chan_list;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Steering
 * -------------------------------------------------------------------------- */

INT wifi_steering_eventRegister(wifi_steering_eventCB_t event_cb)
{
    (void)event_cb;
    return RETURN_OK;
}

INT wifi_steering_clientDisconnect(UINT steeringgroupIndex, INT apIndex,
                                   mac_address_t client_mac,
                                   wifi_disconnectType_t type, UINT reason)
{
    (void)steeringgroupIndex; (void)apIndex; (void)client_mac;
    (void)type; (void)reason;
    return RETURN_OK;
}

/* --------------------------------------------------------------------------
 * Driver-level helpers referenced by librdk_wifihal internals
 * Defined in wifi_hal_priv.h — use void* priv to avoid pulling in
 * hostapd internal headers.
 * -------------------------------------------------------------------------- */

int wifi_setQamPlus(void *priv)
{
    (void)priv;
    return 0;
}

int wifi_setApRetrylimit(void *priv)
{
    (void)priv;
    return 0;
}

/* wifi_drv_get_phy_eht_cap_mac is a void function with hostapd-internal
 * struct args; stub it with a matching signature to satisfy the linker. */
struct eht_capabilities;
struct nlattr;
void wifi_drv_get_phy_eht_cap_mac(struct eht_capabilities *eht_capab,
                                   struct nlattr **tb)
{
    (void)eht_capab; (void)tb;
}

/* --------------------------------------------------------------------------
 * DFS / zero-DFS
 * -------------------------------------------------------------------------- */

INT wifi_setZeroDFSState(UINT radioIndex, BOOL enable, BOOL precac)
{
    (void)radioIndex; (void)enable; (void)precac;
    return RETURN_OK;
}

#endif /* DOCKER_SIM_PORT */
