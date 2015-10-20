/*******************************************************************************
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
*******************************************************************************/
/******************************************************************************/
/**
 *
 * @file xvphy_gthe2.c
 *
 * Contains a minimal set of functions for the XVphy driver that allow access
 * to all of the Video PHY core's functionality. See xvphy.h for a detailed
 * description of the driver.
 *
 * @note	None.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- ---- -------- -----------------------------------------------
 * 1.0   als  10/19/15 Initial release.
 * </pre>
 *
*******************************************************************************/

/******************************* Include Files ********************************/

#include "xvphy_gt.h"
#include "xstatus.h"

/**************************** Function Prototypes *****************************/

static u8 XVphy_MToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId);
static u16 XVphy_NToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, u8 NId);
static u8 XVphy_DToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, XVphy_DirectionType Dir);
static u8 XVphy_DrpEncodeQpllMCpllMN2(u8 AttrEncode);
static u8 XVphy_DrpEncodeCpllN1(u8 AttrEncode);
static u8 XVphy_DrpEncodeCpllTxRxD(u8 AttrEncode);
static u16 XVphy_DrpEncodeQpllN(u8 AttrEncode);

u32 XVphy_Gthe2CfgSetCdr(XVphy *InstancePtr, u8 QuadId, XVphy_ChannelId ChId);
u32 XVphy_Gthe2CheckPllOpRange(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, u64 PllClkOutFreqHz);
u32 XVphy_Gthe2OutDivChReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, XVphy_DirectionType Dir);
u32 XVphy_Gthe2ClkChReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId);
u32 XVphy_Gthe2ClkCmnReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId CmnId);
u32 XVphy_Gthe2RxChReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId);

/************************** Constant Definitions ******************************/

/* DRP register space. */
#define XVPHY_DRP_RXCDR_CFG(n)			(0xA8 + n)

#define XVPHY_DRP_CPLL_PROG              	0x5E
#define XVPHY_DRP_OUT_DIV_PROG           	0x88
#define XVPHY_DRP_QPLL_CFG               	0x32
#define XVPHY_DRP_QPLL_REFCLK_DIV_PROG   	0x33
#define XVPHY_DRP_QPLL_FBDIV_PROG        	0x36
#define XVPHY_DRP_QPLL_FBDIV_RATIO_PROG  	0x37
#define XVPHY_DRP_RXCDR_CFG_WORD0        	0xA8
#define XVPHY_DRP_RXCDR_CFG_WORD1        	0xA9
#define XVPHY_DRP_RXCDR_CFG_WORD2        	0xAA
#define XVPHY_DRP_RXCDR_CFG_WORD3        	0xAB
#define XVPHY_DRP_RXCDR_CFG_WORD4        	0xAC

/* PLL operating ranges. */
#define XVPHY_QPLL_LB_MIN		 8000000000LL
#define XVPHY_QPLL_LB_MAX		13100000000LL
#define XVPHY_QPLL_UB_MIN		 8000000000LL
#define XVPHY_QPLL_UB_MAX		13100000000LL
#define XVPHY_CPLL_MIN			 1600000000LL
#define XVPHY_CPLL_MAX			 5160000000LL

#define XVPHY_DIFF_SWING_DP_L0			0x03
#define XVPHY_DIFF_SWING_DP_L1			0x06
#define XVPHY_DIFF_SWING_DP_L2			0x09
#define XVPHY_DIFF_SWING_DP_L3			0x0F

#define XVPHY_PREEMP_DP_L0			0x00
#define XVPHY_PREEMP_DP_L1			0x0E
#define XVPHY_PREEMP_DP_L2			0x14
#define XVPHY_PREEMP_DP_L3			0x14

const u8 Gthe2CpllDivsM[]	= {1, 2, 0};
const u8 Gthe2CpllDivsN1[]	= {4, 5, 0};
const u8 Gthe2CpllDivsN2[]	= {1, 2, 3, 4, 5, 0};
const u8 Gthe2CpllDivsD[]	= {1, 2, 4, 8, 0};

const u8 Gthe2QpllDivsM[]	= {4, 3, 2, 1, 0};
const u8 Gthe2QpllDivsN1[]	= {16, 20, 32, 40, 64, 66, 80, 100, 0};
const u8 Gthe2QpllDivsN2[]	= {1, 0};
const u8 Gthe2QpllDivsD[]	= {8, 4, 2, 1, 0};

const XVphy_GtConfig Gthe2Config = {
	.CfgSetCdr = XVphy_Gthe2CfgSetCdr,
	.CheckPllOpRange = XVphy_Gthe2CheckPllOpRange,
	.OutDivChReconfig = XVphy_Gthe2OutDivChReconfig,
	.ClkChReconfig = XVphy_Gthe2ClkChReconfig,
	.ClkCmnReconfig = XVphy_Gthe2ClkCmnReconfig,
	.RxChReconfig = XVphy_Gthe2RxChReconfig,
	.TxPllRefClkDiv1Reconfig = NULL,

	.CpllDivs = {
		.M = Gthe2CpllDivsM,
		.N1 = Gthe2CpllDivsN1,
		.N2 = Gthe2CpllDivsN2,
		.D = Gthe2CpllDivsD,
	},
	.QpllDivs = {
		.M = Gthe2QpllDivsM,
		.N1 = Gthe2QpllDivsN1,
		.N2 = Gthe2QpllDivsN2,
		.D = Gthe2QpllDivsD,
	},
};

/**************************** Function Definitions ****************************/

/*****************************************************************************/
/**
* This function will set the clock and data recovery (CDR) values for a given
* channel.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
*
* @return
*		- XST_SUCCESS if the configuration was successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2CfgSetCdr(XVphy *InstancePtr, u8 QuadId, XVphy_ChannelId ChId)
{
	XVphy_Channel *ChPtr;

	/* Set CDR values only for CPLLs. */
	if ((ChId < XVPHY_CHANNEL_ID_CH1) || (ChId > XVPHY_CHANNEL_ID_CH4)) {
		return XST_FAILURE;
	}

	ChPtr = &InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)];

	/* Update the RXCDR_CFG2 settings. */
	ChPtr->PllParams.Cdr[0] = 0x0020;
	ChPtr->PllParams.Cdr[1] = 0x07FE;
	ChPtr->PllParams.Cdr[3] = (ChPtr->RxOutDiv == 1) ? 0xC208 : 0xC220;
	ChPtr->PllParams.Cdr[4] = 0x0018;

	/* RxOutDiv = 1  => Cdr[2] = 0x2000
	 * RxOutDiv = 2  => Cdr[2] = 0x1000
	 * RxOutDiv = 4  => Cdr[2] = 0x0800
	 * RxOutDiv = 8  => Cdr[2] = 0x0400 */
	u8 RxOutDiv = ChPtr->RxOutDiv;
	ChPtr->PllParams.Cdr[2] = 0x2000;
	while (RxOutDiv >>= 1) {
		ChPtr->PllParams.Cdr[2] >>= 1;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function will check if a given PLL output frequency is within the
* operating range of the PLL for the GT type.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
* @param	PllClkOutFreqHz is the frequency to check.
*
* @return
*		- XST_SUCCESS if the frequency resides within the PLL's range.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2CheckPllOpRange(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, u64 PllClkOutFreqHz)
{
	u32 Status = XST_FAILURE;

	if ((ChId == XVPHY_CHANNEL_ID_CMN0) &&
			(XVPHY_QPLL_LB_MIN <= PllClkOutFreqHz) &&
			(PllClkOutFreqHz <= XVPHY_QPLL_LB_MAX)) {
		Status = XST_SUCCESS;
	}
	else if ((ChId >= XVPHY_CHANNEL_ID_CH1) &&
			(ChId <= XVPHY_CHANNEL_ID_CH4) &&
			(XVPHY_CPLL_MIN <= PllClkOutFreqHz) &&
			(PllClkOutFreqHz <= XVPHY_CPLL_MAX)) {
		Status = XST_SUCCESS;
	}

	return Status;
}

/*****************************************************************************/
/**
* This function will set the output divider logic for a given channel.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
* @param	Dir is an indicator for RX or TX.
*
* @return
*		- XST_SUCCESS if the configuration was successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2OutDivChReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, XVphy_DirectionType Dir)
{
	u16 DrpVal;
	u16 WriteVal;

	/* Obtain current DRP register value for TX/RX dividers. */
	DrpVal = XVphy_DrpRead(InstancePtr, QuadId, ChId, 0x88);

	if (Dir == XVPHY_DIR_RX) {
		/* Mask out RX_OUT_DIV. */
		DrpVal &= ~0x07;
		/* Set RX_OUT_DIV. */
		WriteVal = XVphy_DToDrpEncoding(InstancePtr, QuadId, ChId,
								XVPHY_DIR_RX);
		DrpVal |= WriteVal;
	}
	else {
		/* Mask out TX_OUT_DIV. */
		DrpVal &= ~0x70;
		/* Set TX_OUT_DIV. */
		WriteVal = XVphy_DToDrpEncoding(InstancePtr, QuadId, ChId,
								XVPHY_DIR_TX);
		DrpVal |= (WriteVal << 4);
	}
	/* Write new DRP register value for TX/RX dividers. */
	XVphy_DrpWrite(InstancePtr, QuadId, ChId, 0x88, DrpVal);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function will configure the channel clock settings.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
*
* @return
*		- XST_SUCCESS if the configuration was successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2ClkChReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId)
{
	u16 DrpVal;
	u16 WriteVal;

	/* Obtain current DRP register value for PLL dividers. */
	DrpVal = XVphy_DrpRead(InstancePtr, QuadId, ChId, 0x5E);
	/* Mask out clock divider bits. */
	DrpVal &= ~(0x1FFF);
	/* Set CPLL_FBDIV. */
	WriteVal = XVphy_NToDrpEncoding(InstancePtr, QuadId, ChId, 2);
	DrpVal |= WriteVal;
	/* Set CPLL_FBDIV_45. */
	WriteVal = XVphy_NToDrpEncoding(InstancePtr, QuadId, ChId, 1);
	DrpVal |= (WriteVal << 7);
	/* Set CPLL_REFCLKDIV. */
	WriteVal = XVphy_MToDrpEncoding(InstancePtr, QuadId, ChId);
	DrpVal |= (WriteVal << 8);
	/* Write new DRP register value for PLL dividers. */
	XVphy_DrpWrite(InstancePtr, QuadId, ChId, 0x5E, DrpVal);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function will configure the common channel clock settings.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	CmnId is the common channel ID to operate on.
*
* @return
*		- XST_SUCCESS if the configuration was successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2ClkCmnReconfig(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId CmnId)
{
	u16 DrpVal;
	u16 WriteVal;

	/* Obtain current DRP register value for QPLL_CFG. */
	DrpVal = 0x01C1;
	/* Mask out QPLL_CFG. */
	DrpVal &= ~(1 << 6);
	/* Set QPLL_CFG lower/upper band setting to hardware. */
	WriteVal = InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(CmnId)].
		PllParams.IsLowerBand;
	DrpVal |= (WriteVal << 6);
	/* Write new DRP register value for QPLL_CFG. */
	XVphy_DrpWrite(InstancePtr, QuadId, CmnId, 0x32, DrpVal);

	/* Obtain current DRP register value for QPLL_REFCLK_DIV. */
	DrpVal = XVphy_DrpRead(InstancePtr, QuadId, CmnId, 0x33);
	DrpVal |= 0x0068;
	/* Mask out QPLL_REFCLK_DIV. */
	DrpVal &= ~(0xF800);
	/* Set QPLL_REFCLK_DIV. */
	WriteVal = XVphy_MToDrpEncoding(InstancePtr, QuadId, CmnId);
	DrpVal |= (WriteVal << 11);
	/* Write new DRP register value for QPLL_REFCLK_DIV. */
	XVphy_DrpWrite(InstancePtr, QuadId, CmnId, 0x33, DrpVal);

	/* Obtain current DRP register value for QPLL_FBDIV. */
	DrpVal = XVphy_DrpRead(InstancePtr, QuadId, CmnId, 0x36);
	/* Mask out QPLL_FBDIV. */
	DrpVal &= ~(0x3FFF);
	/* Set QPLL_FBDIV. */
	WriteVal = XVphy_NToDrpEncoding(InstancePtr, QuadId, CmnId, 0);
	DrpVal |= WriteVal;
	/* Write new DRP register value for QPLL_FBDIV. */
	XVphy_DrpWrite(InstancePtr, QuadId, CmnId, 0x36, DrpVal);

	/* Obtain current DRP register value for QPLL_FBDIV_RATIO. */
	DrpVal = XVphy_DrpRead(InstancePtr, QuadId, CmnId, 0x37);
	/* Mask out QPLL_FBDIV_RATIO. */
	DrpVal &= ~(1 << 6);
	/* Set QPLL_FBDIV_RATIO. */
	if (InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(CmnId)].PllParams.
			NFbDiv != 66) {
		DrpVal |= (1 << 6);
	}
	/* Write new DRP register value for QPLL_FBDIV_RATIO. */
	XVphy_DrpWrite(InstancePtr, QuadId, CmnId, 0x37, DrpVal);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function will configure the channel's RX settings.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
*
* @return
*		- XST_SUCCESS if the configuration was successful.
*		- XST_FAILURE otherwise.
*
* @note		None.
*
******************************************************************************/
u32 XVphy_Gthe2RxChReconfig(XVphy *InstancePtr, u8 QuadId, XVphy_ChannelId ChId)
{
	XVphy_Channel *ChPtr;
	u16 DrpVal;
	u8 CfgIndex;

	ChPtr = &InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)];

	/* RXCDR_CFG(CfgIndex) */
	for (CfgIndex = 0; CfgIndex < 5; CfgIndex++) {
		DrpVal = ChPtr->PllParams.Cdr[CfgIndex];
		if (!DrpVal) {
			/* Don't modify RX_CDR configuration. */
			continue;
		}
		XVphy_DrpWrite(InstancePtr, QuadId, ChId,
			XVPHY_DRP_RXCDR_CFG(CfgIndex), DrpVal);
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
* This function will translate the configured M value to DRP encoding.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
*
* @return	The DRP encoding for M.
*
* @note		None.
*
******************************************************************************/
static u8 XVphy_MToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId)
{
	u8 MRefClkDiv;
	u8 DrpEncode;

	if ((ChId >= XVPHY_CHANNEL_ID_CH1) && (ChId <= XVPHY_CHANNEL_ID_CH4)) {
		MRefClkDiv = InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)]
			.PllParams.MRefClkDiv;
	}
	else if ((ChId == XVPHY_CHANNEL_ID_CMN0) ||
			(ChId == XVPHY_CHANNEL_ID_CMN1)) {
		MRefClkDiv = InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)]
			.PllParams.MRefClkDiv;
	}
	else {
		MRefClkDiv = 0;
	}

	DrpEncode = XVphy_DrpEncodeQpllMCpllMN2(MRefClkDiv);

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured D value to DRP encoding.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
* @param	Dir is an indicator for RX or TX.
*
* @return	The DRP encoding for D.
*
* @note		None.
*
******************************************************************************/
static u8 XVphy_DToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, XVphy_DirectionType Dir)
{
	u8 OutDiv;
	u8 DrpEncode;

	OutDiv = InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)].
		OutDiv[Dir];

	DrpEncode = XVphy_DrpEncodeCpllTxRxD(OutDiv);

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured N1/N2 value to DRP encoding.
*
* @param	InstancePtr is a pointer to the XVphy core instance.
* @param	QuadId is the GT quad ID to operate on.
* @param	ChId is the channel ID to operate on.
* @param	NId specified to operate on N1 (if == 1) or N2 (if == 2).
*
* @return	The DRP encoding for N1/N2.
*
* @note		None.
*
******************************************************************************/
static u16 XVphy_NToDrpEncoding(XVphy *InstancePtr, u8 QuadId,
		XVphy_ChannelId ChId, u8 NId)
{
	u8 NFbDiv;
	u16 DrpEncode;

	if ((ChId == XVPHY_CHANNEL_ID_CMN0) ||
			(ChId == XVPHY_CHANNEL_ID_CMN1)) {
		NFbDiv = InstancePtr->Quads[QuadId].Plls[XVPHY_CH2IDX(ChId)].
			PllParams.NFbDiv;
		DrpEncode = XVphy_DrpEncodeQpllN(NFbDiv);
	}
	else if (NId == 1) {
		NFbDiv = InstancePtr->Quads[QuadId].Plls[
			XVPHY_CH2IDX(ChId)].PllParams.N1FbDiv;
		DrpEncode = XVphy_DrpEncodeCpllN1(NFbDiv);
	}
	else {
		NFbDiv = InstancePtr->Quads[QuadId].Plls[
			XVPHY_CH2IDX(ChId)].PllParams.N2FbDiv;
		DrpEncode = XVphy_DrpEncodeQpllMCpllMN2(NFbDiv);
	}

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured QPLL's M or CPLL's M or N2
* values to DRP encoding.
*
* @param	AttrEncode is the attribute to encode.
*
* @return	The DRP encoding for the QPLL's M or CPLL's M or N2 values.
*
* @note		None.
*
******************************************************************************/
static u8 XVphy_DrpEncodeQpllMCpllMN2(u8 AttrEncode)
{
	u8 DrpEncode;

	switch (AttrEncode) {
	case 1:
		DrpEncode = 16;
		break;
	case 6:
		DrpEncode = 5;
		break;
	case 10:
		DrpEncode = 7;
		break;
	case 12:
		DrpEncode = 13;
		break;
	case 20:
		DrpEncode = 15;
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 8:
	case 16:
		DrpEncode = (AttrEncode - 2);
		break;
	default:
		DrpEncode = 0xFF;
		break;
	}

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured CPLL's N1 value to DRP encoding.
*
* @param	AttrEncode is the attribute to encode.
*
* @return	The DRP encoding for the CPLL's N1 value.
*
* @note		None.
*
******************************************************************************/
static u8 XVphy_DrpEncodeCpllN1(u8 AttrEncode)
{
	u8 DrpEncode;

	DrpEncode = AttrEncode - 4;

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured CPLL's D values to DRP encoding.
*
* @param	AttrEncode is the attribute to encode.
*
* @return	The DRP encoding for the CPLL's D value.
*
* @note		None.
*
******************************************************************************/
static u8 XVphy_DrpEncodeCpllTxRxD(u8 AttrEncode)
{
	u8 DrpEncode;

	switch (AttrEncode) {
	case 1:
		DrpEncode = 0;
		break;
	case 2:
		DrpEncode = 1;
		break;
	case 4:
		DrpEncode = 2;
		break;
	case 8:
		DrpEncode = 3;
		break;
	case 16:
		DrpEncode = 4;
		break;
	default:
		DrpEncode = 0xFF;
		break;
	}

	return DrpEncode;
}

/*****************************************************************************/
/**
* This function will translate the configured QPLL's N value to DRP encoding.
*
* @param	AttrEncode is the attribute to encode.
*
* @return	The DRP encoding for the QPLL's N value.
*
* @note		None.
*
******************************************************************************/
static u16 XVphy_DrpEncodeQpllN(u8 AttrEncode)
{
	u16 DrpEncode;

	switch (AttrEncode) {
	case 16:
		DrpEncode = 0x020;
		break;
	case 20:
		DrpEncode = 0x030;
		break;
	case 32:
		DrpEncode = 0x060;
		break;
	case 40:
		DrpEncode = 0x080;
		break;
	case 64:
		DrpEncode = 0x0E0;
		break;
	case 66:
		DrpEncode = 0x140;
		break;
	case 80:
		DrpEncode = 0x120;
		break;
	case 100:
		DrpEncode = 0x170;
		break;
	default:
		DrpEncode = 0xFF;
		break;
	}

	return DrpEncode;
}
