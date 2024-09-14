repo=$(pwd)


if [ -d "${1}/project-spec/meta-user/recipes-modules/drm-tmds-pl-drv" ];
then
    echo "drm-tmds-pl-drv already installed, updating files only...";
    cp  ${repo}/drivers/gpu/drm/xlnx/drm-tmds-pl-drv.c \
        ${repo}/drivers/gpu/drm/xlnx/drm-tmds-pl-drv.h \
        ${1}/project-spec/meta-user/recipes-modules/drm-tmds-pl-drv/files/;
else
    echo "Installing drm-tmds-pl-drv module...";
    cd ${1}
    # Create module in Petalinux
    petalinux-create -t modules --name drm-tmds-pl-drv --enable;

    # Copy source files
    cp  ${repo}/drivers/gpu/drm/xlnx/drm-tmds-pl-drv.c \
        ${repo}/drivers/gpu/drm/xlnx/drm-tmds-pl-drv.h \
        ${1}/project-spec/meta-user/recipes-modules/drm-tmds-pl-drv/files/;

    # Add the absolute path of source files to makefile
    echo EXTRA_CFLAGS = -I${1}/project-spec/meta-user/recipes-modules/drm-tmds-pl-drv/files/ \
        >> ${1}/project-spec/meta-user/recipes-modules/drm-tmds-pl-drv/files/Makefile
fi

if [ -d "${1}/project-spec/meta-user/recipes-modules/clk-dglnt-dynclk" ]; then
    echo "Digilent Dynamic Clock module installed, skipping...";
else
    echo "Installing Digilent Dynamic Clock module...";
    cd ${1}
    # Create module in Petalinux and copy the dynamic clock driver
    petalinux-create -t modules --name clk-dglnt-dynclk --enable;
    cp ${repo}/drivers/clk/clk-dglnt-dynclk.c \
        ${1}/project-spec/meta-user/recipes-modules/clk-dglnt-dynclk/files/
fi

echo "drm-tmds-pl-drv installation finished.";

cd ${1}