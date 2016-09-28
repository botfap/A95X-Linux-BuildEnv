/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CUTILS_PROPERTIES_H
#define __CUTILS_PROPERTIES_H

#ifdef __cplusplus
extern "C" {
#endif

/* System properties are *small* name value pairs managed by the
** property service.  If your data doesn't fit in the provided
** space it is not appropriate for a system property.
**
** WARNING: system/bionic/include/sys/system_properties.h also defines
**          these, but with different names.  (TODO: fix that)
*/
#define PROPERTY_KEY_MAX   32
#define PROPERTY_VALUE_MAX  124

#ifdef __cplusplus
}
#endif

#endif
