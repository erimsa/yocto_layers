#ifndef __DRV_HDMITX_H__
#define __DRV_HDMITX_H__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*
 *					INCLUDE DECLARATIONS
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *					MACRO DECLARATIONS
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *					DATA TYPES
 *---------------------------------------------------------------------------*/
enum hdmitx_video_timing_e
{
	HDMITX_VIDEO_TIMING_480P = 0,
	HDMITX_VIDEO_TIMING_576P,
	HDMITX_VIDEO_TIMING_720P60,
	HDMITX_VIDEO_TIMING_1080P60,
	HDMITX_VIDEO_TIMING_MAX
};

/*----------------------------------------------------------------------------*
 *					EXTERNAL DECLARATIONS
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *					FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/
void hdmitx_enable_video(enum hdmitx_video_timing_e timing);

#ifdef __cplusplus
}
#endif


#endif // __DRV_HDMITX_H__