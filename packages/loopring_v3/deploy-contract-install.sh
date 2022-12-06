#!/bin/bash

git submodule update --init --recursive

case "$OSTYPE" in
    linux*)
        echo "OS: Linux"
        cd ethsnarks
        sudo apt-get update
        sudo apt-get -y install make
        sudo make ubuntu-dependencies
        curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.34.0/install.sh | bash
        export NVM_DIR="$HOME/.nvm"
        [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
        nvm install 10
        ;;

    *)
        echo "unsupported OS: $OSTYPE"
        exit 0
        ;;
esac

make PIP_ARGS= python-dependencies
cd ..
npm install
