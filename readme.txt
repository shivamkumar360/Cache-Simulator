
Cache Simulator Project

cd ~/emsdk
source ./emsdk_env.sh
emcc -O3 cache.cpp -o cache.js --bind -s ALLOW_MEMORY_GROWTH=1


Input Format:
Each line should be:
R/W <hex_address>

Example:
R 0x1000
W 0x2000