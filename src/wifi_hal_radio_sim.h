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
 * Docker radio simulation — functions populating the HAL data structures
 * for 3 synthetic radios (2.4 GHz / 5 GHz / 6 GHz) without kernel drivers.
 */

#ifndef WIFI_HAL_RADIO_SIM_H
#define WIFI_HAL_RADIO_SIM_H

/*
 * populate_simulated_radios() - Build the g_wifi_hal radio/interface tables
 *
 * Called from init_nl80211() in place of the nl80211 GET_WIPHY / GET_INTERFACE
 * discovery path when DOCKER_SIM_PORT is defined. Fills
 * g_wifi_hal.radio_info[0..2] for 2.4 GHz, 5 GHz, and 6 GHz and attaches
 * the per-radio interface_map with entries derived from the static
 * interface_index_map table.
 *
 * Returns 0 on success, -1 on allocation failure.
 */
int populate_simulated_radios(void);

#endif /* WIFI_HAL_RADIO_SIM_H */
