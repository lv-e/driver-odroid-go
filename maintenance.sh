
#!/bin/bash
set -e
echo "hi! this is hardkernel's odroid-go driver maintenance script for lv-engine"

buildDockerImage(){
    docker login
    docker build -t lvedock/lve_odroid_go .
    docker push lvedock/lve_odroid_go
}

buildAndRun(){

    tmpdir=$(mktemp -d 2>/dev/null || mktemp -d -t 'tmpdir')
    
    mkdir ${tmpdir}/source

    cp -R ./src/main ${tmpdir}/main
    cp -R ./src/components/engine-link/lv-engine ${tmpdir}/source/lv-engine
    cp -R ./src/components/driver-link/lv-driver ${tmpdir}/source/lv-driver
    cp -R ./src/components/game-link/lv-game ${tmpdir}/source/lv-game

    cp -R ./src/main ${tmpdir}/main
    cp ./src/cmake/main-CMakeLists.txt ${tmpdir}/main/CMakeLists.txt
    cp ./src/cmake/project-CMakeLists.txt ${tmpdir}/CMakeLists.txt
    cp ./src/partitions.csv ${tmpdir}/partitions.csv
    cp ./src/sdkconfig ${tmpdir}/sdkconfig

    cd $tmpdir
    idf.py -p /dev/cu.SLAB_USBtoUART flash
    rm -rf $tmpdir 
}

## what should we do?
while true; do

        echo "available options are:"
        echo "1) build and publish docker image"
        echo "2) build and run on odroid"
        read -p "choose one: " opt; echo "--"

        case $opt in
            1) buildDockerImage; break;;
            2) buildAndRun; break;;
            * ) echo "ok! bye."; exit;;
        esac
done