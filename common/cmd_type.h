#ifndef	_CMD_TYPE_HPP_
#define _CMD_TYPE_HPP_


enum
{
	PAN_RETREAT,
	PAN_RETREAT_CANCEL,
	PAN_LINK_REQ,
	PAN_INSTALL_RF,
	PAN_DISABLE_FUNC,			// to disable substation functionality because of license issue.
	PAN_RM_NRPATH,
	PAN_RM_LICENSE,
	PAN_REBOOT,
	PAN_LINK_INFO,
	PAN_UWB_RETREAT,
	PAN_UWB_RETREAT_CANCEL,
	UWB_GAIN_SETTING,
};

#endif
