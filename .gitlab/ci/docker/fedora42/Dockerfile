FROM fedora:42
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

# Set the default compiler to GCC 14
ENV CC=/usr/bin/gcc-14
ENV CXX=/usr/bin/g++-14

COPY install_adios.sh /root/install_adios.sh
RUN sh /root/install_adios.sh

COPY install_openvr.sh /root/install_openvr.sh
RUN sh /root/install_openvr.sh

COPY install_catalyst.sh /root/install_catalyst.sh
RUN sh /root/install_catalyst.sh

COPY install_ospray.sh /root/install_ospray.sh
RUN sh /root/install_ospray.sh

COPY install_imath.sh /root/install_imath.sh
RUN sh /root/install_imath.sh

COPY install_alembic.sh /root/install_alembic.sh
RUN sh /root/install_alembic.sh

COPY install_anari_helide.sh /root/install_anari_helide.sh
RUN sh /root/install_anari_helide.sh

# XXX(vpn): no stable URL for use in OptiX and MDL SDKs
# COPY install_visrtx.sh /root/install_visrtx.sh
# RUN sh /root/install_visrtx.sh
