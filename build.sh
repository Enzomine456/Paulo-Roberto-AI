#!/bin/bash

# Instalar dependências
apt-get update
apt-get install -y build-essential libzip-dev libssl-dev libxml2-dev libicu-dev

# Baixar e extrair ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.10.0/onnxruntime-linux-x64-1.10.0.tgz
tar -xzf onnxruntime-linux-x64-1.10.0.tgz
export ONNXRUNTIME_DIR=$(pwd)/onnxruntime-linux-x64-1.10.0

# Baixar LibTorch
wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.10.1%2Bcpu.zip
unzip libtorch-cxx11-abi-shared-with-deps-1.10.1+cpu.zip
export Torch_DIR=$(pwd)/libtorch

# Compilar o projeto
g++ -std=c++17 -o paulo_roberto_ai main.cpp \
    -I${ONNXRUNTIME_DIR}/include -L${ONNXRUNTIME_DIR}/lib -lonnxruntime \
    -I${Torch_DIR}/include -I${Torch_DIR}/include/torch/csrc/api/include \
    -L${Torch_DIR}/lib -ltorch -ltorch_cpu -lc10 \
    -lzip -lssl -lcrypto -lxml2 -licuuc -licudata -lhttplib -lpthread

# Criar diretório público
mkdir -p public
cp paulo_roberto_ai public/
chmod +x public/paulo_roberto_ai

# Criar arquivo de inicialização (public/start.sh)
echo '#!/bin/bash
./paulo_roberto_ai 8080 &
while ! nc -z localhost 8080; do
  sleep 0.1
done
' > public/start.sh
chmod +x public/start.sh