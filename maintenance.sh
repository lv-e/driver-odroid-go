
#!/bin/bash
set -e
echo "hi! this is hardkernel's odroid-go driver maintenance script for lv-engine"

buildDockerImage(){
    docker login
    docker build -t lvedock/lve_odroid_go .
    docker push lvedock/lve_odroid_go
}

## what should we do?
while true; do

        echo "available options are:"
        echo "1) build and publish docker image"
        read -p "choose one: " opt; echo "--"

        case $opt in
            1) buildDockerImage; break;;
            * ) echo "ok! bye."; exit;;
        esac
done