/*****************************************************************************
 *
 * MODULE:             JN-AN-1218
 *
 * COMPONENT:          App_DimmableLight.c
 *
 * DESCRIPTION:        ZLO Demo: Dimmable Light Implementation
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
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "zps_gen.h"
#include "AppHardwareApi.h"
#include "app_reporting.h"
#include "App_OnOffCurtain.h"
#include "app_zcl_light_task.h"
#include "app_common.h"
#include "dbg.h"
#include <string.h>
#include "app_light_interpolation.h"
#include "DriverBulb_Shim.h"
#include "bdb_options.h"

#include "zlo_device_id.h"

#include "Z1_KG_MB3.h"
#include "DriverBulb_Shim.h"
#include "app_main.h"
#include "app_reporting.h"

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

tsZLO_DimmableLightDevice sLight;
tsZLO_OnOffLightDevice sOnOffLight[3];
tsIdentifyWhite sIdEffect;
tsCLD_ZllDeviceTable sDeviceTable = { ZLO_NUMBER_DEVICES,
                                      {
                                          { 0,
                                            HA_PROFILE_ID,
                                            ZLO_DIMMABLE_LIGHT_DEVICE_ID,
                                            DIMMABLELIGHT_LIGHT_ENDPOINT,
                                            APPLICATION_DEVICE_VERSION,
                                            0,
                                            0}
                                      }
};

/* define the default reports */
tsReports asDefaultReports[ZLO_NUMBER_OF_REPORTS] = \
{\
    {GENERAL_CLUSTER_ID_ONOFF,           {0, E_ZCL_BOOL,     E_CLD_ONOFF_ATTR_ID_ONOFF,                         0,0xFFFF,0,{0}}},\
    {GENERAL_CLUSTER_ID_LEVEL_CONTROL,   {0, E_ZCL_UINT8,    E_CLD_LEVELCONTROL_ATTR_ID_CURRENT_LEVEL,          0,0xFFFF,0,{10}}}\
};
//tsReports asDefaultReports[ZLO_NUMBER_OF_REPORTS] = \
//{\
//    {GENERAL_CLUSTER_ID_ONOFF,           {0, E_ZCL_BOOL,     E_CLD_ONOFF_ATTR_ID_ONOFF,                         ZLO_MIN_REPORT_INTERVAL,ZLO_MAX_REPORT_INTERVAL,0,{0}}},\
//    {GENERAL_CLUSTER_ID_LEVEL_CONTROL,   {0, E_ZCL_UINT8,    E_CLD_LEVELCONTROL_ATTR_ID_CURRENT_LEVEL,          ZLO_MIN_REPORT_INTERVAL,ZLO_MAX_REPORT_INTERVAL,0,{10}}}\
//};

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
 * Type                        Name                  Descirption
 * tfpZCL_ZCLCallBackFunction  fptr                  Pointer to ZCL Callback function
 *
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PUBLIC teZCL_Status eApp_ZLO_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr)
{
    zps_vSetIgnoreProfileCheck();
    ZPS_vAplZdoRegisterInterPanFilter( bAllowInterPanEp);

    return eZLO_RegisterDimmableLightEndPoint(DIMMABLELIGHT_LIGHT_ENDPOINT,
                                              fptr,
                                              &sLight);
}

PUBLIC teZCL_Status eApp_ZLO_OnOff_RegisterEndpoint_1(tfpZCL_ZCLCallBackFunction fptr)
{

	teZCL_Status status;
    status = eZLO_RegisterOnOffLightEndPoint(DIMMABLELIGHT_LIGHT_1_ENDPOINT,
									fptr,
									&sOnOffLight[0]);
   	return status;

}
PUBLIC teZCL_Status eApp_ZLO_OnOff_RegisterEndpoint_2(tfpZCL_ZCLCallBackFunction fptr)
{

	teZCL_Status status;
    status = eZLO_RegisterOnOffLightEndPoint(DIMMABLELIGHT_LIGHT_2_ENDPOINT,
									fptr,
									&sOnOffLight[1]);
   	return status;

}
PUBLIC teZCL_Status eApp_ZLO_OnOff_RegisterEndpoint_3(tfpZCL_ZCLCallBackFunction fptr)
{

	teZCL_Status status;
    status = eZLO_RegisterOnOffLightEndPoint(DIMMABLELIGHT_LIGHT_3_ENDPOINT,
									fptr,
									&sOnOffLight[2]);
   	return status;

}
//PUBLIC teZCL_Status eApp_ZLO_OnOff_RegisterEndpoint_4(tfpZCL_ZCLCallBackFunction fptr)
//{
//
//	teZCL_Status status;
//    status = eZLO_RegisterOnOffLightEndPoint(DIMMABLELIGHT_LIGHT_4_ENDPOINT,
//									fptr,
//									&sOnOffLight[3]);
//   	return status;
//
//}
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
* RETURNS: True to allow reception, False otherwise
*
****************************************************************************/
PRIVATE bool bAllowInterPanEp(uint8 u8Ep, uint16 u16ProfileId) {

    if ( (u8Ep == DIMMABLELIGHT_LIGHT_ENDPOINT) &&
          ( u16ProfileId == ZLL_PROFILE_ID))
    {
        return TRUE;
    }
    return FALSE;
}


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
PUBLIC void vAPP_ZCL_DeviceSpecific_Init()
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
    return DIMMABLELIGHT_LIGHT_ENDPOINT;
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
    memset(&sLight,0,sizeof(tsZLO_DimmableLightDevice));
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
PUBLIC void APP_vHandleIdentify(uint16 u16Time) {
static bool bActive = FALSE;

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT) {
        /* do nothing */

    }
    else if (u16Time == 0)
    {
        /*
         * Restore to off/off state
         */
    	APP_vSetLed(sLight.sOnOffServerCluster.bOnOff);
//        vSetBulbState( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel);
        bActive = FALSE;
    }
    else
    {
        /* Set the Identify levels */
        if (!bActive) {
            bActive = TRUE;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Count = 5;
            APP_vSetLed(FALSE);
//            vSetBulbState( TRUE, CLD_LEVELCONTROL_MAX_LEVEL );
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
 * PARAMETER: uint8 End Point Identifier
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vIdEffectTick(uint8 u8Endpoint) {

    if (u8Endpoint != DIMMABLELIGHT_LIGHT_ENDPOINT) {
        return;
    }

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
    {
        if (sIdEffect.u8Tick > 0)
        {
            sIdEffect.u8Tick--;
            /* Set the light parameters */
            APP_vSetLed(TRUE);
//            vSetBulbState(TRUE, sIdEffect.u8Level);

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
            APP_vSetLed(sLight.sOnOffServerCluster.bOnOff);
//            vSetBulbState( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel);
        }
    } else {
        if (sLight.sIdentifyServerCluster.u16IdentifyTime) {
            sIdEffect.u8Count--;
            if (0 == sIdEffect.u8Count) {
                sIdEffect.u8Count = 5;
                if (sIdEffect.u8Level) {
                    sIdEffect.u8Level = 0;
                    APP_vSetLed(FALSE);
//                    vSetBulbState( FALSE, 0);
                }
                else
                {
                    sIdEffect.u8Level = 250;
                    APP_vSetLed(TRUE);
//                    vSetBulbState( TRUE, CLD_LEVELCONTROL_MAX_LEVEL);
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
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vStartEffect(uint8 u8Effect) {
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

/****************************************************************************
 *
 * NAME: vSetBulbState
 *
 * DESCRIPTION:
 * ZLL Device Specific build driver interface
 *
 * PARAMETER: the on/off state, the level
 *
 * RETURNS: void
 *
 ****************************************************************************/
#ifdef MONO_WITH_LEVEL
PUBLIC void vSetBulbState(bool bOn, uint8 u8Level)
#elif defined MONO_ON_OFF
PUBLIC void vSetBulbState(bool bOn)
#endif
{
    vBULB_SetOnOff(bOn);
}


#ifndef DR1192

PUBLIC void vISR_Timer3(void)
{
    (void) u8AHI_TimerFired(E_AHI_TIMER_3);
}

PUBLIC void vISR_Timer4(void)
{
    (void) u8AHI_TimerFired(E_AHI_TIMER_4);
}
#endif

#if (!defined DR1175)  && (!defined DR1173) && (!defined OM15045)
PUBLIC void APP_cbTimerButtonScan(void *pvParam)
{

}
#endif

#ifdef DR1190
PUBLIC void vISR_SystemController(void)
{

}
#endif

static uint8_t CurtainCommand=0;

PUBLIC void vApp_StartOpenCurtain(void)
{
	 DriverBulb_bSetNo(BULB_0_VAL);
	 vBULB_SetOnOff(FALSE);
	 DriverBulb_bSetNo(BULB_2_VAL);
	 vBULB_SetOnOff(FALSE);

	 CurtainCommand = 1;
	 //delay 30s
	 ZTIMER_eStart(u8TimerCurtain, ZTIMER_TIME_MSEC(100));
}

PUBLIC void vApp_StopMoveCurtain(void)
{
	 DriverBulb_bSetNo(BULB_0_VAL);
	 vBULB_SetOnOff(FALSE);
	 DriverBulb_bSetNo(BULB_2_VAL);
	 vBULB_SetOnOff(FALSE);

	 CurtainCommand = 3;
	 ZTIMER_eStart(u8TimerCurtain, ZTIMER_TIME_MSEC(100));
}

PUBLIC void vApp_StartCloseCurtain(void)
{
	 DriverBulb_bSetNo(BULB_0_VAL);
	 vBULB_SetOnOff(FALSE);
	 DriverBulb_bSetNo(BULB_2_VAL);
	 vBULB_SetOnOff(FALSE);

	 CurtainCommand = 5;
	 //delay 30s
	 ZTIMER_eStart(u8TimerCurtain, ZTIMER_TIME_MSEC(100));
}

PUBLIC void APP_cbTimerCurtain(void){
	ZTIMER_eStop(u8TimerCurtain);

	switch(CurtainCommand){
	case 1://����
		CurtainCommand = 2;
		DriverBulb_bSetNo(BULB_0_VAL);
		vBULB_SetOnOff(TRUE);
		DriverBulb_bSetNo(BULB_2_VAL);
		vBULB_SetOnOff(FALSE);
		ZTIMER_eStart(u8TimerCurtain, ZTIMER_TIME_MSEC(30000));
		break;
	case 2://��������
		CurtainCommand = 0;
		DriverBulb_bSetNo(BULB_0_VAL);
		vBULB_SetOnOff(FALSE);
		DriverBulb_bSetNo(BULB_2_VAL);
		vBULB_SetOnOff(FALSE);
		APP_ImmediatelyReportingOnOff(DIMMABLELIGHT_LIGHT_ENDPOINT);
		break;
	case 3://ֹͣ
		APP_ImmediatelyReportingOnOff(DIMMABLELIGHT_LIGHT_1_ENDPOINT);
		break;

	case 5://�ر�
		CurtainCommand = 6;
		DriverBulb_bSetNo(BULB_0_VAL);
		vBULB_SetOnOff(FALSE);
		DriverBulb_bSetNo(BULB_2_VAL);
		vBULB_SetOnOff(TRUE);
		ZTIMER_eStart(u8TimerCurtain, ZTIMER_TIME_MSEC(30000));
		break;
	case 6://�رս���
		CurtainCommand = 0;
		DriverBulb_bSetNo(BULB_0_VAL);
		vBULB_SetOnOff(FALSE);
		DriverBulb_bSetNo(BULB_2_VAL);
		vBULB_SetOnOff(FALSE);
		APP_ImmediatelyReportingOnOff(DIMMABLELIGHT_LIGHT_2_ENDPOINT);
		break;
	default:
		DriverBulb_bSetNo(BULB_0_VAL);
		vBULB_SetOnOff(FALSE);
		DriverBulb_bSetNo(BULB_2_VAL);
		vBULB_SetOnOff(FALSE);
		break;
	}

}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
