#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>

#include <linux/component.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>

#include "xlnx_bridge.h"
#include "xlnx_crtc.h"
#include "xlnx_drv.h"

#ifndef DRM_TMDS_PL_DRV
#define DRM_TMDS_PL_DRV

static int _create_connector(struct drm_encoder *encoder);

static int drm_tmds_pl_bind(struct device *, struct device *, void *);
static void drm_tmds_pl_unbind(struct device *, struct device *, void *);
static void drm_tmds_pl_atomic_mode_set(struct drm_encoder *, struct drm_crtc_state *, struct drm_connector_state *);
static void drm_tmds_pl_enable(struct drm_encoder *);
static void drm_tmds_pl_disable(struct drm_encoder *);
static enum drm_connector_status drm_tmds_pl_connector_detect(struct drm_connector *, bool);
static void drm_tmds_pl_connector_destroy(struct drm_connector *);
static int drm_tmds_pl_get_modes(struct drm_connector *);
static int drm_tmds_pl_mode_valid(struct drm_connector *, struct drm_display_mode *);
static struct drm_encoder *drm_tmds_pl_best_encoder(struct drm_connector *);

#endif