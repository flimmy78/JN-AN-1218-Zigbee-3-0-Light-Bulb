/*****************************************************************************
 *
 * MODULE:             JN-AN-1218
 *
 * COMPONENT:          App_ColorTemperatureLight.c
 *
 * DESCRIPTION:        ZLO Demo: Color Temperature Light Implementation
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2016. All rights reserved
 *
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "zps_gen.h"

/* Application includes */
#include "app_reporting.h"
#include "App_ColorTemperatureLight.h"
#include "app_common.h"
#include "app_zcl_light_task.h"
#include "dbg.h"
#include <string.h>
#include "bdb_options.h"

#include "AppHardwareApi.h"
#include "app_light_interpolation.h"
#include "DriverBulb_Shim.h"
#include "DriverBulb.h"

#include "zlo_device_id.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_LIGHT_TASK
#define TRACE_LIGHT_TASK  TRUE
#else
#define TRACE_LIGHT_TASK FALSE
#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
tsZLO_ColourTemperatureLightDevice sLight;
tsIdentifyWhite sIdEffect;
tsCLD_ZllDeviceTable sDeviceTable = { ZLO_NUMBER_DEVICES,
                                      {
                                          { 0,
                                            HA_PROFILE_ID,
                                            ZLO_COLOUR_TEMPERATURE_LIGHT_DEVICE_ID,
                                            COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                                            APPLICATION_DEVICE_VERSION,
                                            0,
                                            0}
                                      }
};

/* define the default reports */
tsReports asDefaultReports[ZLO_NUMBER_OF_REPORTS] =
{
    {GENERAL_CLUSTER_ID_ONOFF,           {0, E_ZCL_BOOL,     E_CLD_ONOFF_ATTR_ID_ONOFF,                         ZLO_MIN_REPORT_INTERVAL,ZLO_MAX_REPORT_INTERVAL,0,{0}}},
    {GENERAL_CLUSTER_ID_LEVEL_CONTROL,   {0, E_ZCL_UINT8,    E_CLD_LEVELCONTROL_ATTR_ID_CURRENT_LEVEL,          ZLO_MIN_REPORT_INTERVAL,ZLO_MAX_REPORT_INTERVAL,0,{10}}},
    {LIGHTING_CLUSTER_ID_COLOUR_CONTROL, {0, E_ZCL_UINT16, E_CLD_COLOURCONTROL_ATTR_COLOUR_TEMPERATURE_MIRED, ZLO_MIN_REPORT_INTERVAL,ZLO_MAX_REPORT_INTERVAL,0,{10}}}
};

/*
 * If overriding the ZLL Master Key in the appilation,
 * define BDB_APPLICATION_DEFINED_TL_MASTER_KEY in bdb_options.h
 * otherwize the key defined in BDB\Source\touchlink\bdb_tl_common.c will be used.
 */
#ifdef BDB_APPLICATION_DEFINED_TL_MASTER_KEY
PUBLIC tsReg128 sTLMasterKey = {0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00 };
#endif

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool bAllowInterPanEp(uint8 u8Ep, uint16 u16ProfileId);

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: eApp_ZLO_RegisterEndpoint
 *
 * DESCRIPTION:
 * Register ZLO endpoints
 *
 * PARAMETER
 * Type                        Name                    Description
 * tfpZCL_ZCLCallBackFunction  fptr                    Pointer to ZCL Callback function
 * t
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PUBLIC teZCL_Status eApp_ZLO_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr)
{

    zps_vSetIgnoreProfileCheck();
    ZPS_vAplZdoRegisterInterPanFilter( bAllowInterPanEp);

    return eZLO_RegisterColourTemperatureLightEndPoint(COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                                                      fptr,
                                                      &sLight);
}

/****************************************************************************
*
* NAME: bAllowInterPanEp
*
* DESCRIPTION: Allows the application to decide which end point receive
* inter pan messages
*
*
* PARAMETER: the end point receiving the inter pan
*
* RETURNS: void
*
****************************************************************************/
PRIVATE bool bAllowInterPanEp(uint8 u8Ep, uint16 u16ProfileId) {

    if ( (u8Ep == COLORTEMPERATURELIGHT_LIGHT_ENDPOINT) &&
          ( u16ProfileId == ZLL_PROFILE_ID))
    {
        return TRUE;
    }
    return FALSE;
}

#if(!defined DR1221) && (!defined DR1221_Dimic)
/****************************************************************************
 *
 * NAME: vApp_eCLD_ColourControl_GetRGB
 *
 * DESCRIPTION:
 * To get RGB value
 *
 * PARAMETER
 * Type         Name               Descirption
 * uint8 *      pu8Red             Pointer to Red in RGB value
 * uint8 *      pu8Green           Pointer to Green in RGB value
 * uint8 *      pu8Blue            Pointer to Blue in RGB value
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_eCLD_ColourControl_GetRGB(uint8 *pu8Red,uint8 *pu8Green,uint8 *pu8Blue)
{
    eCLD_ColourControl_GetRGB( COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                              pu8Red,
                               pu8Green,
                              pu8Blue);
}
#endif

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZLO Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vAPP_ZCL_DeviceSpecific_Init(void)
{
    /* Initialise the strings in Basic */
    uint8    au8PCode[CLD_BAS_PCODE_SIZE] = BAS_PCODE_STRING;
    memcpy(sLight.sBasicServerCluster.au8ManufacturerName, BAS_MANUF_NAME_STRING, CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLight.sBasicServerCluster.au8ModelIdentifier, BAS_MODEL_ID_STRING, CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLight.sBasicServerCluster.au8DateCode, BAS_DATE_STRING, CLD_BAS_DATE_SIZE);
    memcpy(sLight.sBasicServerCluster.au8SWBuildID, BAS_SW_BUILD_STRING, CLD_BAS_SW_BUILD_SIZE);
    memcpy(sLight.sBasicServerCluster.au8ProductURL, BAS_URL_STRING, CLD_BAS_URL_SIZE);
    memcpy(sLight.sBasicServerCluster.au8ProductCode, au8PCode, CLD_BAS_PCODE_SIZE);

    sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
    sIdEffect.u8Tick = 0;

    /* Set the initial value to 4000 Kelvin */
    sLight.sColourControlServerCluster.u16ColourTemperatureMired = (1E6/4000);

#if (defined DR1221) || (defined DR1221_Dimic)  || (defined OM15045)
    {
        uint16 u16PhyMax;
        uint16 u16PhyMin;

        DriverBulb_vGetColourTempPhyMinMax(&u16PhyMin,&u16PhyMax);
        sLight.sColourControlServerCluster.u16ColourTemperatureMiredPhyMin = u16PhyMin;
        sLight.sColourControlServerCluster.u16ColourTemperatureMiredPhyMax = u16PhyMax;
    }
#endif


}

/****************************************************************************
 *
 * NAME: app_u8GetDeviceEndpoint
 *
 * DESCRIPTION:
 * Return the application endpoint
 *
 * PARAMETER: void
 *
 * RETURNS: uint8
 *
 ****************************************************************************/
PUBLIC uint8 app_u8GetDeviceEndpoint( void)
{
    return COLORTEMPERATURELIGHT_LIGHT_ENDPOINT;
}

/****************************************************************************
 *
 * NAME: vApp_ZCL_ResetDeviceStructure
 *
 * DESCRIPTION:
 * Resets the device structure
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vApp_ZCL_ResetDeviceStructure(void)
{
    memset(&sLight,0,sizeof(tsZLO_ColourTemperatureLightDevice));
}
/****************************************************************************
 *
 * NAME: APP_vHandleIdentify
 *
 * DESCRIPTION:
 * ZLO Device Specific identify
 *
 * PARAMETER: the identify time
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void APP_vHandleIdentify(uint16 u16Time)
{
    static bool bActive = FALSE;

#if(!defined DR1221) && (!defined DR1221_Dimic)
    uint8 u8Red, u8Green, u8Blue;
    vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);
    #endif

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT) {
        /* do nothing */
    }
    else if (u16Time == 0)
    {
        /*
         * Restore to off/off state
         */

        #if (defined DR1221) || (defined DR1221_Dimic)  || (defined OM15045)/* Restore the CCT TW bulb to pre-identify state */


        vTunableWhiteLightSetLevels(sLight.sOnOffServerCluster.bOnOff,
                                    sLight.sLevelControlServerCluster.u8CurrentLevel,
                                    sLight.sColourControlServerCluster.u16ColourTemperatureMired);

        #else /* Restore CCT RGB bulb to pre-identify State */
        vRGBLight_SetLevels(sLight.sOnOffServerCluster.bOnOff,
                            sLight.sLevelControlServerCluster.u8CurrentLevel,
                            u8Red,
                            u8Green,
                            u8Blue);
        #endif
        bActive = FALSE;
    }
    else
    {
        /* Set the Identify levels */
        if (!bActive) {
            bActive = TRUE;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Count = 5;
            #if (defined DR1221) || (defined DR1221_Dimic)  || (defined OM15045) /* Restore the CCT TW bulb to pre-identify state */
            vTunableWhiteLightSetLevels(TRUE,
                                        CLD_LEVELCONTROL_MAX_LEVEL,
                                        sLight.sColourControlServerCluster.u16ColourTemperatureMired);

            #else /* Restore CCT RGB bulb to pre-identify State */
            vRGBLight_SetLevels(TRUE,
                                CLD_LEVELCONTROL_MAX_LEVEL,
                                u8Red,
                                u8Green,
                                u8Blue);
            #endif
        }
    }
}

/****************************************************************************
 *
 * NAME: vIdEffectTick
 *
 * DESCRIPTION:
 * ZLO Device Specific identify tick
 *
 * PARAMETER: uin8 End Point identifier
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vIdEffectTick(uint8 u8Endpoint) {

#if(!defined DR1221) && (!defined DR1221_Dimic)
    uint8 u8Red, u8Green, u8Blue;
    vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);
    #endif

    if (u8Endpoint != COLORTEMPERATURELIGHT_LIGHT_ENDPOINT) {
        return;
    }

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
    {
        if (sIdEffect.u8Tick > 0)
        {
            sIdEffect.u8Tick--;
            /* Set the light parameters */
            #if (defined DR1221) || (defined DR1221_Dimic)  || (defined OM15045)
            vTunableWhiteLightSetLevels(TRUE,sIdEffect.u8Level,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
            #else
            vRGBLight_SetLevels(TRUE, sIdEffect.u8Level, u8Red, u8Green, u8Blue);
            #endif

            /* Now adjust parameters ready for for next round */
            switch (sIdEffect.u8Effect) {
                case E_CLD_IDENTIFY_EFFECT_BLINK:
                    break;

                case E_CLD_IDENTIFY_EFFECT_BREATHE:
                    if (sIdEffect.bDirection) {
                        if (sIdEffect.u8Level >= 250) {
                            sIdEffect.u8Level -= 50;
                            sIdEffect.bDirection = 0;
                        } else {
                            sIdEffect.u8Level += 50;
                        }
                    } else {
                        if (sIdEffect.u8Level == 0) {
                            // go back up, check for stop
                            sIdEffect.u8Count--;
                            if ((sIdEffect.u8Count) && ( !sIdEffect.bFinish)) {
                                sIdEffect.u8Level += 50;
                                sIdEffect.bDirection = 1;
                            } else {
                                //DBG_vPrintf(TRACE_LIGHT_TASK, "\n>>set tick 0<<");
                                /* lpsw2773 - stop the effect on the next tick */
                                sIdEffect.u8Tick = 0;
                            }
                        } else {
                            sIdEffect.u8Level -= 50;
                        }
                    }
                    break;
                case E_CLD_IDENTIFY_EFFECT_OKAY:
                    if ((sIdEffect.u8Tick == 10) || (sIdEffect.u8Tick == 5)) {
                        sIdEffect.u8Level ^= 254;
                        if (sIdEffect.bFinish) {
                            sIdEffect.u8Tick = 0;
                        }
                    }
                    break;
                case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
                    if ( sIdEffect.u8Tick == 74) {
                        sIdEffect.u8Level = 1;
                        if (sIdEffect.bFinish) {
                            sIdEffect.u8Tick = 0;
                        }
                    }
                    break;
                default:
                    if ( sIdEffect.bFinish ) {
                        sIdEffect.u8Tick = 0;
                    }
                }
        } else {
            /*
             * Effect finished, restore the light
             */

            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
            sIdEffect.bDirection = FALSE;
            APP_ZCL_vSetIdentifyTime(0);
            #if (defined DR1221) || (defined DR1221_Dimic) || (defined OM15045)
            vTunableWhiteLightSetLevels( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
            #else
            vRGBLight_SetLevels( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel, u8Red, u8Green, u8Blue);
            #endif
        }
    } else {
        if (sLight.sIdentifyServerCluster.u16IdentifyTime) {
            sIdEffect.u8Count--;
            if (0 == sIdEffect.u8Count) {
                sIdEffect.u8Count = 5;
                if (sIdEffect.u8Level) {
                    sIdEffect.u8Level = 0;

                    #if (defined DR1221) || (defined DR1221_Dimic) || (defined OM15045)
                    vTunableWhiteLightSetLevels( FALSE, 0,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
                    #else
                    vRGBLight_SetLevels( FALSE, 0, u8Red, u8Green, u8Blue);
                    #endif
                }
                else
                {
                    sIdEffect.u8Level = 250;
                    #if (defined DR1221) || (defined DR1221_Dimic)  || (defined OM15045)
                    vTunableWhiteLightSetLevels( TRUE, CLD_LEVELCONTROL_MAX_LEVEL,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
                    #else
                    vRGBLight_SetLevels( TRUE, CLD_LEVELCONTROL_MAX_LEVEL, u8Red, u8Green, u8Blue);
                    #endif
                }
            }
        }
    }

}

/****************************************************************************
 *
 * NAME: vStartEffect
 *
 * DESCRIPTION:
 * ZLO Device Specific identify effect set up
 *
 * PARAMETER: uint8 Identify Effect id
 *
 * RETURNS: void
 *
 ****************************************************************************/

PUBLIC void vStartEffect(uint8 u8Effect)
{
    switch (u8Effect) {
        case E_CLD_IDENTIFY_EFFECT_BLINK:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BLINK;
            if (sLight.sOnOffServerCluster.bOnOff) {
                sIdEffect.u8Level = 0;
            } else {
                sIdEffect.u8Level = 250;
            }
            sIdEffect.bFinish = FALSE;
            APP_ZCL_vSetIdentifyTime(2);
            sIdEffect.u8Tick = 10;
            break;
        case E_CLD_IDENTIFY_EFFECT_BREATHE:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BREATHE;
            sIdEffect.bDirection = 1;
            sIdEffect.bFinish = FALSE;
            sIdEffect.u8Level = 0;
            sIdEffect.u8Count = 15;
            APP_ZCL_vSetIdentifyTime(17);
            sIdEffect.u8Tick = 200;
            break;
        case E_CLD_IDENTIFY_EFFECT_OKAY:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_OKAY;
            sIdEffect.bFinish = FALSE;
            if (sLight.sOnOffServerCluster.bOnOff) {
                sIdEffect.u8Level = 0;
            } else {
                sIdEffect.u8Level = 254;
            }
            APP_ZCL_vSetIdentifyTime(3);
            sIdEffect.u8Tick = 15;
            break;
        case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE;
            sIdEffect.u8Level = 254;
            sIdEffect.bFinish = FALSE;
            APP_ZCL_vSetIdentifyTime(9);
            sIdEffect.u8Tick = 80;
            break;

        case E_CLD_IDENTIFY_EFFECT_FINISH_EFFECT:
            if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
            {
                DBG_vPrintf(TRACE_LIGHT_TASK, "\n<FINISH>");
                sIdEffect.bFinish = TRUE;
            }
            break;
        case E_CLD_IDENTIFY_EFFECT_STOP_EFFECT:
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
            APP_ZCL_vSetIdentifyTime(1);
            break;
    }
}

#if  !((defined DR1221) || (defined DR1221_Dimic) || (defined OM15045))
/****************************************************************************
 *
 * NAME: vRGBLight_SetLevels
 *
 * DESCRIPTION:
 * Set the RGB and levels
 *
 * PARAMETER: the on/off state, the level, red, green and blue values
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRGBLight_SetLevels(bool_t bOn, uint8 u8Level, uint8 u8Red, uint8 u8Green, uint8 u8Blue)
{
     if (bOn == TRUE)
     {
         vLI_Start(u8Level, u8Red, u8Green, u8Blue,0);
     } 
     else 
     {
        vLI_Stop();  
    }
     vBULB_SetOnOff(bOn);
}
#endif


#if ((defined DR1221) || (defined DR1221_Dimic) || (defined OM15045))
/****************************************************************************
 *
 * NAME: vTunableWhiteLightSetLevels
 *
 * DESCRIPTION:
 * Device Specific build driver interface
 *
 * PARAMETER: the on/off state, the level, and colour temperature
 *
 * RETURNS: void
 ****************************************************************************/

PUBLIC void vTunableWhiteLightSetLevels(bool_t bOn, uint32 u32Level, int32 i32ColourTemperature)
{
    if (bOn) /* only move the colour temperature if lamp is on */
    {
        vLI_Start(u32Level, 0,0,0,(1E6/i32ColourTemperature));
    }
    else
    {
        vLI_Stop();
    }
    vBULB_SetOnOff(bOn);
}
#endif

/*****************************************************************************/
/* OS config sheet has extra ISR for real bulbs so include stubs on DK4      */
/*  bulbs to allow builds                                                    */
/*****************************************************************************/
#if (defined DR1175) || (defined DR1173)
PUBLIC void vISR_Timer3(void)
{
    (void) u8AHI_TimerFired(E_AHI_TIMER_3);
}

PUBLIC void vISR_Timer4(void)
{
    (void) u8AHI_TimerFired(E_AHI_TIMER_4);
}
#endif

/*****************************************************************************/
/* Real bulb DR1221 re-purposes system controller interrupt which is used    */
/* for button press detection on DK4. This is OK as real bulbs don't have    */
/* buttons (DR1221 uses is for Rebroadcast flicker prevention & mains synch  */
/*****************************************************************************/
#if (defined DR1221) || (defined DR1221_Dimic)
PUBLIC void APP_cbTimerButtonScan(void *pvParam)
{
    (void) u32AHI_DioWakeStatus();
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

