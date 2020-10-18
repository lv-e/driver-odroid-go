
FROM lvedock/lve_runtime

# compiler setup

# copy files from source to 
COPY src/components/engine-link/lv-engine /lv/lv-engine
COPY src/components/engine-link/CMakeLists.txt /lv/lv-engine/CMakeLists.txt

COPY src/components/driver-link/lv-driver /lv/lv-driver