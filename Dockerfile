
FROM espressif/idf

# add lv-cli

RUN apt-get update && apt-get install -y \
    nodejs npm

RUN npm i -g \
    @lv-game-editor/lv-cli@"<2.0.0" \
    @lv-game-editor/encoder-lvcode@"<2.0.0" \
    @lv-game-editor/encoder-png@"<2.0.0" \
    @lv-game-editor/encoder-lvproject@"<2.0.0" 


# folder structure convention

RUN \
# the same as the project folder on host machine
    mkdir -p -m 777 /lv/project \ 
\
# the end of encoding pipeline, where the c++ source are put
    mkdir -p -m 777 /lv/source \
    mkdir -p -m 777 /lv/source/lv-game \
    mkdir -p -m 777 /lv/source/lv-game/artifacts \
\
# used to share information between machines, .shared on host
    mkdir -p -m 777 /lv/shared \
\
# scripts is where build and run .sh files are located
    mkdir -p -m 777 /lv/scripts \
\
# this is the output dir, when needed. also is the .bin dir at host machine
    mkdir -p -m 777 /lv/bin

#
## copy engine & driver & game source files
#

COPY src/main /lv/main
COPY src/components/engine-link/lv-engine /lv/source/lv-engine
COPY src/components/driver-link/lv-driver /lv/source/lv-driver

# copy cmake & ninja files

COPY src/cmake/main-CMakeLists.txt /lv/main/CMakeLists.txt
COPY src/cmake/project-CMakeLists.txt /lv/CMakeLists.txt
COPY src/partitions.csv /lv/partitions.csv
COPY src/sdkconfig /lv/sdkconfig