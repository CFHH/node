#!/bin/bash

rm -r ./out/Release/lib
if [ ! -d ./out/Release/lib  ];then
  mkdir ./out/Release/lib
fi

ar -r ./out/Release/lib/libicudata.a ./out/Release/obj.target/icustubdata/deps/icu-small/source/stubdata/*.o

ar -r ./out/Release/lib/libicui18n.a ./out/Release/obj.target/icui18n/deps/icu-small/source/i18n/*.o

ar -r ./out/Release/lib/libicustubdata.a ./out/Release/obj.target/icustubdata/deps/icu-small/source/stubdata/*.o

ar -r ./out/Release/lib/libicutools.a \
	./out/Release/obj.host/icutools/deps/icu-small/source/common/*.o\
	./out/Release/obj.host/icutools/deps/icu-small/source/i18n/*.o\
	./out/Release/obj.host/icutools/deps/icu-small/source/stubdata/*.o\
	./out/Release/obj.host/icutools/deps/icu-small/source/tools/toolutil/*.o\

ar -r ./out/Release/lib/libicuucx.a ./out/Release/obj.target/icuucx/deps/icu-small/source/common/*.o

ar -r ./out/Release/lib/libv8_base.a ./out/Release/obj.target/v8_base/deps/v8/src/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/asmjs/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/ast/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/builtins/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/compiler/*.o\
		./out/Release/obj.target/v8_base/deps/v8/src/compiler/x64/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/compiler-dispatcher/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/debug/*.o\
		./out/Release/obj.target/v8_base/deps/v8/src/debug/x64/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/extensions/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/heap/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/ic/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/inspector/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/interpreter/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/objects/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/parsing/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/profiler/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/regexp/*.o\
		./out/Release/obj.target/v8_base/deps/v8/src/regexp/x64/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/runtime/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/snapshot/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/tracing/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/trap-handler/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/wasm/*.o\
		./out/Release/obj.target/v8_base/deps/v8/src/wasm/baseline/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/x64/*.o\
	./out/Release/obj.target/v8_base/deps/v8/src/zone/*.o\
	./out/Release/obj.target/v8_base/gen/*.o\
	./out/Release/obj.target/v8_base/gen/src/inspector/protocol/*.o

ar -r ./out/Release/lib/libv8_init.a ./out/Release/obj.target/v8_init/deps/v8/src/*.o

ar -r ./out/Release/lib/libv8_initializers.a ./out/Release/obj.target/v8_initializers/deps/v8/src/builtins/*.o\
	./out/Release/obj.target/v8_initializers/deps/v8/src/builtins/x64/*.o\
	./out/Release/obj.target/v8_initializers/deps/v8/src/heap/*.o\
	./out/Release/obj.target/v8_initializers/deps/v8/src/ic/*.o\
	./out/Release/obj.target/v8_initializers/deps/v8/src/interpreter/*.o

ar -r ./out/Release/lib/libv8_libbase.a ./out/Release/obj.target/v8_libbase/deps/v8/src/base/*.o\
	./out/Release/obj.target/v8_libbase/deps/v8/src/base/debug/*.o\
	./out/Release/obj.target/v8_libbase/deps/v8/src/base/platform/*.o\
	./out/Release/obj.target/v8_libbase/deps/v8/src/base/utils/*.o

ar -r ./out/Release/lib/libv8_libplatform.a ./out/Release/obj.target/v8_libplatform/deps/v8/src/libplatform/*.o\
	./out/Release/obj.target/v8_libplatform/deps/v8/src/libplatform/tracing/*.o

ar -r ./out/Release/lib/libv8_libsampler.a ./out/Release/obj.target/v8_libsampler/deps/v8/src/libsampler/*.o

ar -r ./out/Release/lib/libv8_snapshot.a ./out/Release/obj.target/v8_snapshot/deps/v8/src/*.o\
	./out/Release/obj.target/v8_snapshot/gen/*.o\
	./out/Release/obj.target/v8_snapshot/geni/*.o

ar -r ./out/Release/lib/libzlib.a ./out/Release/obj.target/zlib/deps/zlib/*.o

