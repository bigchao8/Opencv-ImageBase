cd main
g++ *.cpp -o img_preproc -std=c++11 -lpthread `pkg-config --cflags --libs opencv`