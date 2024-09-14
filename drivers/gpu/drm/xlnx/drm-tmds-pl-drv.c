#include "drm-tmds-pl-drv.h"

#define DEFAULT_SETTING_MAX_FREQ 150000  //KHz
#define DEFAULT_SETTING_MAX_H 1920
#define DEFAULT_SETTING_MAX_V 1080
#define DEFAULT_SETTING_PREF_H 1280
#define DEFAULT_SETTING_PREF_V 720

struct drm_tmds_pl {
    struct drm_encoder encoder;
    struct drm_connector connector;
    struct device *dev;
    struct i2c_adapter *i2c_adapter;
    bool i2c_present;
    u32 fmax;
    u32 hmax;
    u32 vmax;
    u32 hpref;
    u32 vpref;
};

#define get_tmds_from_encoder(c) container_of(c, struct drm_tmds_pl, encoder)
#define get_tmds_from_connector(c) container_of(c, struct drm_tmds_pl, connector)

//========== callback registration structures =========

static const struct component_ops drm_tmds_pl_component_ops = {
    .bind = drm_tmds_pl_bind,
    .unbind = drm_tmds_pl_unbind,
};

static const struct drm_encoder_helper_funcs drm_tmds_pl_encoder_helper_funcs = {
    .atomic_mode_set = drm_tmds_pl_atomic_mode_set,
    .enable = drm_tmds_pl_enable,
    .disable = drm_tmds_pl_disable,
};

static const struct drm_encoder_funcs drm_tmds_pl_encoder_funcs = {
    .destroy = drm_encoder_cleanup,
};

static const struct drm_connector_funcs drm_tmds_pl_connector_funcs = {
    .dpms = drm_helper_connector_dpms,
    .detect = drm_tmds_pl_connector_detect,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = drm_tmds_pl_connector_destroy,
    .atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
    .reset = drm_atomic_helper_connector_reset,
};

static struct drm_connector_helper_funcs drm_tmds_pl_connector_helper_funcs = {
    .get_modes = drm_tmds_pl_get_modes,
    .mode_valid = drm_tmds_pl_mode_valid,
    .best_encoder = drm_tmds_pl_best_encoder,
};

//=========== DRM callback functions ============

/**
 * Called by the component framework when finds match
 * prototype: /driver/gpu/drm/drm_encoder.c
 * @dev: device with tmds_pl_drv
 */
static int drm_tmds_pl_bind(struct device *dev, 
                            struct device *master, 
                            void *data)                         
{
    struct drm_tmds_pl *tmds = dev_get_drvdata(dev);
    struct drm_encoder *encoder = &tmds->encoder;
    struct drm_device *drm_dev = data;

    encoder->possible_crtcs = 1;

    // Init the tmds encoder, set its drm device as this one with tmds_pl_drv,
    // register the encoder and encoder_helper funcs, and set the name
    drm_encoder_init(drm_dev, encoder, &drm_tmds_pl_encoder_funcs, DRM_MODE_ENCODER_TMDS, "DRM_TMDS_PL_ENCODER");
    drm_encoder_helper_add(encoder, &drm_tmds_pl_encoder_helper_funcs);

    // Generate an HDMI-A type connector and attach it to the encoder
    // the attaching job is done within the private func
    int ret;
    ret = _create_connector(encoder);
    if (ret)
    {
        dev_err(tmds->dev, "fail calling _create_connector(encoder), ret = %d\n", ret);
        drm_encoder_cleanup(encoder);
    }

    return ret;
}

static void drm_tmds_pl_unbind(struct device *dev, struct device *master, void *data)
{
    struct drm_tmds_pl *tmds = dev_get_drvdata(dev);
    drm_encoder_cleanup(&tmds->encoder);
    drm_connector_cleanup(&tmds->connector);
}

static void drm_tmds_pl_atomic_mode_set(struct drm_encoder *encoder,
                                        struct drm_crtc_state *crtc_state,
                                        struct drm_connector_state *connector_state)
{
    DRM_INFO("DIGILENT SET MODE");
}

static void drm_tmds_pl_enable(struct drm_encoder *encoder)
{
    DRM_INFO("DIGILENT SET MODE");
}

static void drm_tmds_pl_disable(struct drm_encoder *encoder)
{
    DRM_INFO("DIGILENT SET MODE");
}

/**
 * Check to see if anything is attached to the connector.
 * prototype: /include/drm/drm_connector.h
 * @connector: the connector attached to the encoder of this device
 */
static enum drm_connector_status drm_tmds_pl_connector_detect(struct drm_connector *connector, bool force)
{
    struct drm_tmds_pl *tmds = get_tmds_from_connector(connector);

    if (tmds->i2c_present) {
        if (drm_probe_ddc(tmds->i2c_adapter))
            return connector_status_connected;
        return connector_status_disconnected;
    }
    else
        return connector_status_unknown;
}

static void drm_tmds_pl_connector_destroy(struct drm_connector *connector)
{
    drm_connector_unregister(connector);
    drm_connector_cleanup(connector);
    connector->dev = NULL;
}

/**
 * The modeset helper calls this func to detect all supported display mode
 * of the panel, and fill them into the connector->probed_mode list.
 * prototype: /include/drm/drm_modeset_helper_vtables.h
 */
static int drm_tmds_pl_get_modes(struct drm_connector *connector)
{
    struct drm_tmds_pl *tmds = get_tmds_from_connector(connector);
    struct edid *edid;
    int num_modes = 0;

    // Try read EDID from I2C bus
    if (tmds->i2c_present) {
        edid = drm_get_edid(connector, tmds->i2c_adapter);

        drm_mode_connector_update_edid_property(connector, edid);
        if (edid) {
            num_modes = drm_add_edid_modes(connector, edid);        // This fills the connector->probed_mode
            kfree(edid);
        }
    } else {
        // Use default setting, possibly read from device tree by probe()

        // This would add every standards mode, 
        // of which the h/v doesn't surpass hmax or vmax, 
        // to the probed_mode.
        num_modes = drm_add_modes_noedid(connector, tmds->hmax, tmds->vmax);

        drm_set_preferred_mode(connector, tmds->hpref, tmds->vpref);
    }
    return num_modes;
}

static int drm_tmds_pl_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
{
    struct drm_tmds_pl *tmds = get_tmds_from_connector(connector);
    if (mode &&
       !(mode->flags & ((DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK) | DRM_MODE_FLAG_3D_MASK)) &&
       (mode->clock <= tmds->fmax) &&
       (mode->hdisplay <= tmds->hmax) &&
       (mode->vdisplay <= tmds->vmax)) {
        return MODE_OK;
    }

    return MODE_BAD;
}

static struct drm_encoder *drm_tmds_pl_best_encoder(struct drm_connector *connector)
{
    struct drm_tmds_pl *tmds = get_tmds_from_connector(connector);

    return &tmds->encoder;
}

//=========== private functions =============

static int _create_connector(struct drm_encoder *encoder)
{
    struct drm_tmds_pl *tmds = get_tmds_from_encoder(encoder);
    struct drm_connector *connector = &tmds->connector;
    int ret;

    //connector->polled = DRM_CONNECTOR_POLL_HPD;
    connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;

    ret = drm_connector_init(encoder->dev, connector,
                            &drm_tmds_pl_connector_funcs,
                            DRM_MODE_CONNECTOR_HDMIA);
    if (ret) {
        dev_err(tmds->dev, "Failed to initialize connector with drm\n");
        return ret;
    }

    drm_connector_helper_add(connector, &drm_tmds_pl_connector_helper_funcs);
    ret = drm_connector_register(connector);
    if (ret) {
        dev_err(tmds->dev, "Failed to register the connector (ret=%d)\n", ret);
        return ret;
    }
    ret = drm_mode_connector_attach_encoder(connector, encoder);
    if (ret) {
        dev_err(tmds->dev, "Failed to attach encoder to connector (ret=%d)\n", ret);
        return ret;
    }

    return 0;
}

//=========== register driver ==============

static int drm_tmds_pl_probe(struct platform_device *pdev)
{
    struct drm_tmds_pl *tmds;
    struct device_node *sub_node;
    int ret;

    tmds = devm_kzalloc(&pdev->dev, sizeof(*tmds), GFP_KERNEL);
    if (!tmds)
        return -ENOMEM;

    tmds->dev = &pdev->dev;
    platform_set_drvdata(pdev, tmds);

    // Load I2C adapter from the subnode of the device tree
    tmds->i2c_present = false;
    sub_node = of_parse_phandle(pdev->dev.of_node, "drm_tmds_pl,edid-i2c", 0);
    if (sub_node) {
        tmds->i2c_adapter = of_find_i2c_adapter_by_node(sub_node);
        if (!tmds->i2c_adapter)
            dev_info(tmds->dev, "Failed to get the edid i2c adapter, using default modes\n");
        else
            tmds->i2c_present = true;
        of_node_put(sub_node);
    }

    // Try read display preference from device tree.
    // Some parameters would be used in drm_tmds_pl_get_modes().
    ret = of_property_read_u32(pdev->dev.of_node, "setting,fmax", &tmds->fmax);
    if (ret < 0) {
        tmds->fmax = DEFAULT_SETTING_MAX_FREQ;
        dev_info(tmds->dev, "No max frequency in DT, using default %dKHz\n", DEFAULT_SETTING_MAX_FREQ);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "setting,hmax", &tmds->hmax);
    if (ret < 0) {
        tmds->hmax = DEFAULT_SETTING_MAX_H;
        dev_info(tmds->dev, "No max horizontal width in DT, using default %d\n", DEFAULT_SETTING_MAX_H);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "setting,vmax", &tmds->vmax);
    if (ret < 0) {
        tmds->vmax = DEFAULT_SETTING_MAX_V;
        dev_info(tmds->dev, "No max vertical height in DT, using default %d\n", DEFAULT_SETTING_MAX_V);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "setting,hpref", &tmds->hpref);
    if (ret < 0) {
        tmds->hpref = DEFAULT_SETTING_PREF_H;
        if (!tmds->i2c_present)
            dev_info(tmds->dev, "No pref horizontal width in DT, using default %d\n", DEFAULT_SETTING_PREF_H);
    }

    ret = of_property_read_u32(pdev->dev.of_node, "setting,vpref", &tmds->vpref);
    if (ret < 0) {
        tmds->vpref = DEFAULT_SETTING_PREF_V;
        if (!tmds->i2c_present)
            dev_info(tmds->dev, "No pref vertical height in DT, using default %d\n", DEFAULT_SETTING_PREF_V);
    }

    // Add the encoder+connector as a component to the framework
    ret = component_add(tmds->dev, &drm_tmds_pl_component_ops);

    dev_info(tmds->dev, "drm_tmds_pl_drv probed %d\n", ret);
    return ret;
}

static int drm_tmds_pl_remove(struct platform_device *pdev)
{
    struct drm_tmds_pl *tmds = platform_get_drvdata(pdev);
    component_del(tmds->dev, &drm_tmds_pl_component_ops);
    return 0;
}

static const struct of_device_id drm_tmds_encoder_of_match[] = {
    { .compatible = "tmds_pl_drv"},
    { },
};
MODULE_DEVICE_TABLE(of, drm_tmds_encoder_of_match);

static struct platform_driver drm_tmds_pl_drv = {
    .probe = drm_tmds_pl_probe,
    .remove = drm_tmds_pl_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "drm_tmds_pl_drv",
        .of_match_table = drm_tmds_encoder_of_match,
    },
};

module_platform_driver(drm_tmds_pl_drv);

MODULE_AUTHOR("Zhiyang Cao <tinesharp@outlook.com>");
MODULE_DESCRIPTION("DRM Encoder & Connector Driver for TMDS Pipeline on PL");
MODULE_LICENSE("GPL v2");