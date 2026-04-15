
Cache Simulator Project

cd ~/emsdk
source ./emsdk_env.sh
emcc -O3 cache.cpp -o cache.js --bind -s ALLOW_MEMORY_GROWTH=1