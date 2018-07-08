cd main
g++ *.cpp -o img_preproc -std=c++11 -lpthread `pkg-config --cflags --libs opencv`
cd .. 
cd box-format
g++ box-format2.cpp -o box-format -std=c++11 -lpthread `pkg-config --cflags --libs opencv`
