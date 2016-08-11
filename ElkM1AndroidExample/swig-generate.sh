#!/bin/bash
cd ../ElkM1API
swig -O -Wall -DELKM1API -c++ -java -o ../ElkM1AndroidExample/app/src/main/jni/swig_wrap_java.cpp -package elkm1api -outdir ../ElkM1AndroidExample/app/src/main/java/elkm1api swig.i
