/** 
 * @file    GoMaterial.h
 * @brief   Declares the GoMaterial class. 
 * @deprecated
 * @internal
 * Copyright (C) 2016-2019 by LMI Technologies Inc.
 * Licensed under the MIT License.
 * Redistributed files must retain the above copyright notice.
 */
#ifndef GO_MATERIAL_H
#define GO_MATERIAL_H

#include <GoSdk/GoSdkDef.h>
#include <GoSdk/GoAdvanced.h>

/**
 * @deprecated
 * @class   GoMaterial
 * @extends kObject
 * @note    Supported with G1, G2
 * @ingroup GoSdk
 * @brief   Represents configurable material acquisition settings.
 */
typedef GoAdvanced GoMaterial;

/** 
 * @deprecated Sets the material acquisition type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   type       The material type to set.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetType(GoMaterial material, GoMaterialType type);

/** 
 * @deprecated Returns the user defined material acquisition type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The material type.
 */
GoFx(GoMaterialType) GoMaterial_Type(GoMaterial material);

/** 
 * @deprecated Returns a boolean relating to whether the user defined material acquisition type value will be used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if the user defined material type will be used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsTypeUsed(GoMaterial material);

/** 
 * @deprecated Returns the material acquisition type to be used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The system value material type.
 */
GoFx(GoMaterialType) GoMaterial_TypeSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the spot threshold.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Spot threshold.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetSpotThreshold(GoMaterial material, k32u value);

/** 
 * @deprecated Returns the user defined spot threshold.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The spot threshold.
 */
GoFx(k32u) GoMaterial_SpotThreshold(GoMaterial material);

/** 
 * @deprecated Returns the minimum spot threshold limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The minimum spot threshold.
 */
GoFx(k32u) GoMaterial_SpotThresholdLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the maximum spot threshold limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot threshold.
 */
GoFx(k32u) GoMaterial_SpotThresholdLimitMax(GoMaterial material);

/** 
 * @deprecated Returns a boolean value representing whether the user specified spot threshold value is used.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if it is used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsSpotThresholdUsed(GoMaterial material);

/** 
 * @deprecated Returns the system spot threshold value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The system spot threshold.
 */
GoFx(k32u) GoMaterial_SpotThresholdSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the maximum spot width.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Maximum spot width.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetSpotWidthMax(GoMaterial material, k32u value);

/** 
 * @deprecated Returns the user defined maximum spot width.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot width.
 */
GoFx(k32u) GoMaterial_SpotWidthMax(GoMaterial material);

/** 
 * @deprecated Returns the maximum spot width minimum limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot width minimum limit.
 */
GoFx(k32u) GoMaterial_SpotWidthMaxLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the maximum spot width maximum limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot width maximum limit.
 */
GoFx(k32u) GoMaterial_SpotWidthMaxLimitMax(GoMaterial material);

/** 
 * @deprecated Returns a boolean relating to whether the user defined spot width max value will be used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if the user value will be used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsSpotWidthMaxUsed(GoMaterial material);

/** 
 * @deprecated Returns the maximum spot width system value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot width system value.
 */
GoFx(k32u) GoMaterial_SpotWidthMaxSystemValue(GoMaterial material);

/** 
 * @deprecated Returns the number of spot selection type options.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @return             The spot selection type option count.
 */
GoFx(kSize) GoMaterial_SpotSelectionTypeOptionCount(GoMaterial material);

/** 
 * @deprecated Returns the spot selection type option at the given index.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @param   index      The option list index to access.
 * @return             The spot selection type option at the given index.
 */
GoFx(GoSpotSelectionType) GoMaterial_SpotSelectionTypeOptionAt(GoMaterial material, kSize index);

/** 
 * @deprecated Sets the spot selection type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   type       Spot selection type.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetSpotSelectionType(GoMaterial material, GoSpotSelectionType type);

/** 
 * @deprecated Returns the user defined spot selection type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The maximum spot width.
 */
GoFx(GoSpotSelectionType) GoMaterial_SpotSelectionType(GoMaterial material);

/** 
 * @deprecated Returns a boolean relating to whether the user defined spot selection type will be used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if the user value will be used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsSpotSelectionTypeUsed(GoMaterial material);

/** 
 * @deprecated Returns the system spot selection type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             System spot selection type.
 */
GoFx(GoSpotSelectionType) GoMaterial_SpotSelectionTypeSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the analog camera gain.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Analog camera gain.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetCameraGainAnalog(GoMaterial material, k64f value);

/** 
 * @deprecated Returns the user defined analog camera gain value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Analog camera gain value.
 */
GoFx(k64f) GoMaterial_CameraGainAnalog(GoMaterial material);

/** 
 * @deprecated Returns the analog camera gain minimum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Analog camera gain minimum value limit.
 */
GoFx(k64f) GoMaterial_CameraGainAnalogLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the analog camera gain maximum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Analog camera gain maximum value limit.
 */
GoFx(k64f) GoMaterial_CameraGainAnalogLimitMax(GoMaterial material);

/** 
 * @deprecated Returns a boolean value representing whether the user defined analog camera gain is used.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if the user defined analog camera gain is used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsCameraGainAnalogUsed(GoMaterial material);

/** 
 * @deprecated Returns the analog camera gain system value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The analog camera gain system value.
 */
GoFx(k64f) GoMaterial_CameraGainAnalogSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the digital camera gain
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Digital camera gain.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetCameraGainDigital(GoMaterial material, k64f value);

/** 
 * @deprecated Returns the user defined digital camera gain value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The digital camera gain system value.
 */
GoFx(k64f) GoMaterial_CameraGainDigital(GoMaterial material);

/** 
 * @deprecated Returns the digital camera gain minimum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Digital camera gain minimum value limit.
 */
GoFx(k64f) GoMaterial_CameraGainDigitalLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the digital camera gain maximum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Digital camera gain maximum value limit.
 */
GoFx(k64f) GoMaterial_CameraGainDigitalLimitMax(GoMaterial material);

/** 
 * @deprecated Returns a boolean value representing whether the user's digital camera gain value is used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsCameraGainDigitalUsed(GoMaterial material);

/** 
 * @deprecated Returns the system's digital camera gain value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Digital camera gain system value.
 */
GoFx(k64f) GoMaterial_CameraGainDigitalSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the dynamic sensitivity.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Dynamic sensitivity.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetDynamicSensitivity(GoMaterial material, k64f value);

/** 
 * @deprecated Returns the user defined dynamic sensitivity value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             User defined dynamic sensitivity value.
 */
GoFx(k64f) GoMaterial_DynamicSensitivity(GoMaterial material);

/** 
 * @deprecated Returns the dynamic sensitivity minimum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic sensitivity minimum value limit.
 */
GoFx(k64f) GoMaterial_DynamicSensitivityLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the dynamic sensitivity maximum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic sensitivity maximum value limit.
 */
GoFx(k64f) GoMaterial_DynamicSensitivityLimitMax(GoMaterial material);

/** 
 * @deprecated Returns a boolean representing whether the user defined dynamic sensitivity value is used.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsDynamicSensitivityUsed(GoMaterial material);

/** 
 * @deprecated Returns the dynamic sensitivity system value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic sensitivity system value.
 */
GoFx(k64f) GoMaterial_DynamicSensitivitySystemValue(GoMaterial material);

/** 
 * @deprecated Sets the dynamic threshold.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Dynamic threshold.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetDynamicThreshold(GoMaterial material, k32u value);

/** 
 * @deprecated Returns the dynamic threshold minimum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic threshold minimum value limit.
 */
GoFx(k32u) GoMaterial_DynamicThresholdLimitMin(GoMaterial material);

/** 
 * @deprecated Returns the dynamic threshold maximum value limit.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic threshold maximum value limit.
 */
GoFx(k32u) GoMaterial_DynamicThresholdLimitMax(GoMaterial material);

/** 
 * @deprecated Returns the user defined dynamic threshold value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The user defined dynamic threshold value.
 */
GoFx(k32u) GoMaterial_DynamicThreshold(GoMaterial material);

/** 
 * @deprecated Returns a boolean representing whether or not the user defined dynamic threshold is used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic threshold minimum value limit.
 */
GoFx(kBool) GoMaterial_IsDynamicThresholdUsed(GoMaterial material);

/** 
 * @deprecated Returns the dynamic threshold system value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             Dynamic threshold system value.
 */
GoFx(k32u) GoMaterial_DynamicThresholdSystemValue(GoMaterial material);

/** 
 * @deprecated Sets the gamma type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @param   value      Gamma type.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_SetGammaType(GoMaterial material, GoGammaType value);

/** 
 * @deprecated Returns the user defined gamma type.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             User defined gamma type.
 */
GoFx(GoGammaType) GoMaterial_GammaType(GoMaterial material);

/** 
 * @deprecated Returns a boolean representing whether the user defined gamma type is used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             kTRUE if used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsGammaTypeUsed(GoMaterial material);

/** 
 * @deprecated Returns the system's gamma type value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.1.3.106
 * @param   material   GoMaterial object.
 * @return             The system gamma type value.
 */
GoFx(GoGammaType) GoMaterial_GammaTypeSystemValue(GoMaterial material);

/** 
 * @deprecated Enables or disables senstivity compensation. NOTE: This is only applicable to 
 * 2300 B series sensors.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @param   value      kTRUE to enable and kFALSE to disable.
 * @return             Operation status.
 */
GoFx(kStatus) GoMaterial_EnableSensitivityCompensation(GoMaterial material, kBool value);

/** 
 * @deprecated Returns the user defined sensitivity compensation value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @return             User defined sensitivity compensation.
 */
GoFx(kBool) GoMaterial_SensitivityCompensationEnabled(GoMaterial material);

/** 
 * @deprecated Returns a boolean representing whether the user defined sensitivity compensation is used by the system.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @return             kTRUE if used and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_IsSensitivityCompensationEnabledUsed(GoMaterial material);

/** 
 * @deprecated Returns the system's sensitivity compensation value.
 *
 * @public             @memberof GoMaterial
 * @note               Supported with G1, G2
 * @version            Introduced in firmware 4.3.3.124
 * @param   material   GoMaterial object.
 * @return             kTRUE if enabled and kFALSE otherwise.
 */
GoFx(kBool) GoMaterial_SensitivityCompensationEnabledSystemValue(GoMaterial material);

#endif
