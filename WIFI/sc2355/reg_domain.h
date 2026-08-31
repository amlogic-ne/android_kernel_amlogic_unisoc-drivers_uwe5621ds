#ifndef __REG_DOMAIN_H__
#define __REG_DOMAIN_H__
#include <linux/types.h>
//#include "wl_core.h"
#include "msg.h"
#include "cfg80211.h"
//#include "wl_intf.h"
#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0))
#define NL80211_RRF_AUTO_BW 0
#endif

#define SPRD_REG_RULE(start, end, bw, reg_flags) REG_RULE(start, end, bw, 0, 0, reg_flags)
struct sprd_regdomain {
	char country_code[4];
	const struct ieee80211_regdomain *prRegdRules;
};

const struct ieee80211_regdomain *getRegdomainFromSprdDB(char *alpha2);
void apply_custom_regulatory(struct wiphy *pWiphy,
							const struct ieee80211_regdomain *pRegdom);
void ShowChannel(struct wiphy *pWiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
const struct ieee80211_reg_rule *sprd_freq_reg_info_regd(u32 center_freq,
		   const struct ieee80211_regdomain *regd, u32 bw);
#else
const struct ieee80211_reg_rule *
sprd_freq_reg_info_regd(u32 center_freq,
		   const struct ieee80211_regdomain *regd);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
void sprdwl_wiphy_apply_custom_regulatory(struct wiphy *wiphy,
				   const struct ieee80211_regdomain *regd);
#endif
#endif
