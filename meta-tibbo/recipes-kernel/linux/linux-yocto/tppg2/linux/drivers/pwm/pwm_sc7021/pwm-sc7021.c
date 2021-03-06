/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2018 by Sunplus Inc.                             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Inc. All rights are reserved by Sunplus Inc.                          *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Inc. reserves the right to modify this software               *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus Inc.                                                          *
 *  19, Innovation First Road, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file pwm-sc7021.c
 * @brief linux kernel pwm driver
 * @author PoChou Chen
 */
/**************************************************************************
 *                         H E A D E R   F I L E S
 **************************************************************************/
/* #define DEBUG */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/clk.h>
#include "pwm-sc7021.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define SC7021_PWM_NUM		ePWM_MAX

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define to_sc7021_pwm(chip)	container_of(chip, struct sc7021_pwm, chip)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct sc7021_pwm {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
};

enum {
	ePWM0,
	ePWM1,
	ePWM2,
	ePWM3,
	ePWM4,
	ePWM5,
	ePWM6,
	ePWM7,
	ePWM_MAX
};

enum {
	ePWM_DD0,
	ePWM_DD1,
	ePWM_DD2,
	ePWM_DD3,
	ePWM_DD_MAX
};

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int _sc7021_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm);
static void _sc7021_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm);
static int _sc7021_pwm_config(struct pwm_chip *chip,
		struct pwm_device *pwm,
		int duty_ns,
		int period_ns);

static int _sc7021_pwm_probe(struct platform_device *pdev);
static int _sc7021_pwm_remove(struct platform_device *pdev);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const struct pwm_ops _sc7021_pwm_ops = {
	.enable = _sc7021_pwm_enable,
	.disable = _sc7021_pwm_disable,
	.config = _sc7021_pwm_config,
	.owner = THIS_MODULE,
};

static const struct of_device_id _sc7021_pwm_dt_ids[] = {
	{ .compatible = "sunplus,sc7021-pwm", },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, _sc7021_pwm_dt_ids);

static struct platform_driver _sc7021_pwm_driver = {
	.probe		= _sc7021_pwm_probe,
	.remove		= _sc7021_pwm_remove,
	.driver		= {
		.name	= "sc7021-pwm",
		.of_match_table = _sc7021_pwm_dt_ids,
	},
};
module_platform_driver(_sc7021_pwm_driver);

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void _sc7021_reg_init(void *base)
{
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)base;

	pPWMReg->grp244_0 = 0x0000;
	pPWMReg->grp244_1 = 0x0f00;
	pPWMReg->pwm_dd[0].idx_all = 0x0000;
	pPWMReg->pwm_dd[1].idx_all = 0x0000;
	pPWMReg->pwm_dd[2].idx_all = 0x0000;
	pPWMReg->pwm_dd[3].idx_all = 0x0000;
	pPWMReg->pwm_du[0].idx_all = 0x0000;
	pPWMReg->pwm_du[1].idx_all = 0x0000;
	pPWMReg->pwm_du[2].idx_all = 0x0000;
	pPWMReg->pwm_du[3].idx_all = 0x0000;
	pPWMReg->pwm_du[4].idx_all = 0x0000;
	pPWMReg->pwm_du[5].idx_all = 0x0000;
	pPWMReg->pwm_du[6].idx_all = 0x0000;
	pPWMReg->pwm_du[7].idx_all = 0x0000;
}

static void _sc7021_savepwmclk(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sc7021_pwm *pdata = to_sc7021_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	u32 dd_sel = pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel;
	int i;

	if (!(pPWMReg->grp244_1 & (1 << dd_sel)))
		return;

	for (i = 0; i < ePWM_MAX; ++i) {
		if ((pPWMReg->grp244_0 & (1 << i))
			&& (pPWMReg->pwm_du[i].pwm_du_dd_sel == dd_sel))
			break;
	}

	if (i == ePWM_MAX) {
		pPWMReg->grp244_1 &= ~(1 << dd_sel);
		dev_dbg(chip->dev, "save pwm clk:%d dd_sel:%d\n",
			pwm->hwpwm, dd_sel);
	}
}

static int _sc7021_setpwm(struct pwm_chip *chip,
		struct pwm_device *pwm,
		int duty_ns,
		int period_ns)
{
	struct sc7021_pwm *pdata = to_sc7021_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	u32 dd_sel_old = ePWM_DD_MAX, dd_sel_new = ePWM_DD_MAX;
	u32 duty = 0, dd_freq = 0x80;
	u32 tmp2;
	u64 tmp;
	int i;

	tmp = clk_get_rate(pdata->clk) * (u64)period_ns;
	dd_freq = (u32)div64_u64(tmp, 256000000000ULL);

	dev_dbg(chip->dev, "set pwm:%d source clk rate:%llu duty_freq:0x%x(%d)\n",
			pwm->hwpwm, tmp, dd_freq, dd_freq);

	if (dd_freq == 0)
		return -EINVAL;

	if (pPWMReg->grp244_1 & (1 << pwm->hwpwm))
		dd_sel_old = pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel;
	else
		dd_sel_old = ePWM_DD_MAX;

	/* find the same freq and turnon clk source */
	for (i = 0; i < ePWM_DD_MAX; ++i) {
		if ((pPWMReg->grp244_1 & (1 << i))
			&& (pPWMReg->pwm_dd[i].dd == dd_freq))
			break;
	}
	if (i != ePWM_DD_MAX)
		dd_sel_new = i;

	/* dd_sel only myself used */
	if (dd_sel_new == ePWM_DD_MAX) {
		for (i = 0; i < ePWM_MAX; ++i) {
			if (i == pwm->hwpwm)
				continue;

			tmp2 = pPWMReg->pwm_du[i].pwm_du_dd_sel;
			if ((pPWMReg->grp244_1 & (1 << i))
					&& (tmp2 == dd_sel_old))
				break;
		}
		if (i == ePWM_MAX)
			dd_sel_new = dd_sel_old;
	}

	/* find unused clk source */
	if (dd_sel_new == ePWM_DD_MAX) {
		for (i = 0; i < ePWM_DD_MAX; ++i) {
			if (!(pPWMReg->grp244_1 & (1 << i)))
				break;
		}
		dd_sel_new = i;
	}

	if (dd_sel_new == ePWM_DD_MAX) {
		dev_err(chip->dev, "pwm%d Can't found clk source[0x%x(%d)/256].\n",
				pwm->hwpwm, dd_freq, dd_freq);
		return -EBUSY;
	}

	pPWMReg->grp244_1 |= (1 << dd_sel_new);
	pPWMReg->pwm_dd[dd_sel_new].dd = dd_freq;

	dev_dbg(chip->dev, "%s:%d found clk source:%d and set [0x%x(%d)/256]\n",
			__func__, __LINE__, dd_sel_new, dd_freq, dd_freq);

	if (duty_ns == period_ns) {
		pPWMReg->pwm_bypass |= (1 << pwm->hwpwm);
		dev_dbg(chip->dev, "%s:%d set pwm:%d bypass duty\n",
				__func__, __LINE__, pwm->hwpwm);
	} else {
		pPWMReg->pwm_bypass &= ~(1 << pwm->hwpwm);

		tmp = (u64)duty_ns * 256 + (period_ns >> 1);
		duty = (u32)div_u64(tmp, (u32)period_ns);

		dev_dbg(chip->dev, "%s:%d set pwm:%d duty:0x%x(%d)\n",
				__func__, __LINE__, pwm->hwpwm, duty, duty);

		pPWMReg->pwm_du[pwm->hwpwm].pwm_du = duty;
		pPWMReg->pwm_du[pwm->hwpwm].pwm_du_dd_sel = dd_sel_new;
	}

	if ((dd_sel_old != dd_sel_new) && (dd_sel_old != ePWM_DD_MAX))
		pPWMReg->grp244_1 &= ~(1 << dd_sel_old);

	dev_dbg(chip->dev, "%s:%d pwm:%d, output freq:%lu Hz, duty:%u%%\n",
			__func__, __LINE__,
			pwm->hwpwm, clk_get_rate(pdata->clk) / (dd_freq * 256),
			(duty * 100) / 256);

	return 0;
}

static int _sc7021_pwm_config(struct pwm_chip *chip,
		struct pwm_device *pwm,
		int duty_ns,
		int period_ns)
{
	dev_dbg(chip->dev, "%s:%d set pwm:%d enable:%d duty_ns:%d period_ns:%d\n",
			__func__, __LINE__,
			pwm->hwpwm, pwm->state.enabled,
			duty_ns, period_ns);

	if (pwm->state.enabled) {
		if (_sc7021_setpwm(chip, pwm, duty_ns, period_ns))
			return -EBUSY;
	}

	return 0;
}

static int _sc7021_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sc7021_pwm *pdata = to_sc7021_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;
	u32 period = pwm->state.period;
	u32 duty_cycle = pwm->state.duty_cycle;

	dev_dbg(chip->dev, "%s:%d duty_ns:%d period_ns:%d\n",
			__func__, __LINE__,
			duty_cycle, period);

	if (!_sc7021_setpwm(chip, pwm, duty_cycle, period))
		pPWMReg->pwm_en |= (1 << pwm->hwpwm);
	else
		return -EBUSY;

	return 0;
}

static void _sc7021_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct sc7021_pwm *pdata = to_sc7021_pwm(chip);
	struct _PWM_REG_ *pPWMReg = (struct _PWM_REG_ *)pdata->base;

	pPWMReg->pwm_en &= ~(1 << pwm->hwpwm);
	_sc7021_savepwmclk(chip, pwm);
}

static int _sc7021_pwm_probe(struct platform_device *pdev)
{
	struct sc7021_pwm *pdata;
	struct resource *res;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	if (!np) {
		dev_err(&pdev->dev, "invalid devicetree node\n");
		return -EINVAL;
	}

	if (!of_device_is_available(np)) {
		dev_err(&pdev->dev, "devicetree status is not available\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		dev_err(&pdev->dev, "get resource memory from devicetree node\n");
		return PTR_ERR(res);
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL)
		return -ENOMEM;

	pdata->chip.dev = &pdev->dev;
	pdata->chip.ops = &_sc7021_pwm_ops;
	pdata->chip.npwm = SC7021_PWM_NUM;
	/* pwm cell = 2 (of_pwm_simple_xlate) */
	pdata->chip.of_xlate = NULL;

	pdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pdata->base)) {
		dev_err(&pdev->dev, "mapping resource memory\n");
		return PTR_ERR(pdata->base);
	}

	pdata->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pdata->clk)) {
		dev_err(&pdev->dev, "not found clk source\n");
		return PTR_ERR(pdata->clk);
	}

	/* init module reg */
	_sc7021_reg_init(pdata->base);

	ret = pwmchip_add(&pdata->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add PWM chip\n");
		return ret;
	}

	platform_set_drvdata(pdev, pdata);

	return ret;
}

static int _sc7021_pwm_remove(struct platform_device *pdev)
{
	struct sc7021_pwm *pdata;

	pdata = platform_get_drvdata(pdev);
	if (pdata == NULL)
		return -ENODEV;

	return pwmchip_remove(&pdata->chip);
}

MODULE_DESCRIPTION("SC7021 PWM Driver");
MODULE_AUTHOR("PoChou Chen <pochou.chen@sunplus.com>");
MODULE_LICENSE("GPL");

