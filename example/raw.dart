import 'dart:ffi' as ffi;
import 'dart:ffi';
import 'dart:isolate';
import 'package:ffi/ffi.dart';
import 'dart:io' show Directory, exit, sleep;
import 'package:path/path.dart' as path;
import '../lib/generated_bindings.dart' as toby_binding;

final libraryPath = path.join(Directory.current.path, '.', 'toby.so');
final dylib = ffi.DynamicLibrary.open(libraryPath);
final toby = toby_binding.NativeLibrary(dylib);

void hostOn(int argc, ffi.Pointer<ffi.Pointer<ffi.Char>> argv) {
  print("tobyHostOn - argc : $argc");
  for (int i = 0; i < argc; i++) {
    print("tobyHostOn - argv[$i] = ${argv[i].cast<Utf8>().toDartString()}");
  }
}

void tobyOnLoad(ffi.Pointer<ffi.Void> isolate) {
  print('** topyOnLoad : ${isolate.address.toRadixString(16)}');

  toby.tobyHostOn(
      "exit".toNativeUtf8().cast(), ffi.Pointer.fromFunction(hostOn));

  ffi.Pointer<Utf8> source = """
    function _f(x) {
      return x ? x : ':)';
    };
    var _v = 43;
  """
      .toNativeUtf8();

  // fixme : safe
  var dest = malloc<Uint8>(1024);
  int ret = toby.tobyJSCompile(source.cast(), dest.cast(), 1024);
  if (ret < 0) {
    print(
        "** tobyJSCompile error - code : $ret , data : ${dest.cast<Utf8>().toDartString()}");
  } else {
    print("** tobyJSCompile : ${dest.cast<Utf8>().toDartString()}");
  }

  ret = toby.tobyJSCall(
      "_f".toNativeUtf8().cast(), "".toNativeUtf8().cast(), dest.cast(), 1024);
  if (ret < 0) {
    print(
        "** tobyJSCall error - code : $ret , data : ${dest.cast<Utf8>().toDartString()}");
  } else {
    print("** tobyJSCall : ${dest.cast<Utf8>().toDartString()}");
  }

  malloc.free(dest);
}

void tobyOnUnload(ffi.Pointer<ffi.Void> isolate, int exitCode) {
  print(
      '** tobyOnUnload : ${isolate.address.toRadixString(16)} exitCode : $exitCode');

  exit(exitCode);
}

ffi.Pointer<ffi.Char> tobyHostCall(
    ffi.Pointer<ffi.Char> name, ffi.Pointer<ffi.Char> value) {
  print(
      '** from javascript. name = ${name.cast<Utf8>().toDartString()} , value = ${value.cast<Utf8>().toDartString()}');
  return 'hi there'.toNativeUtf8().cast();
}

_toby(SendPort sendPort) async {
  var processName = 'example';
  var userScript = "require('./app.js');";

  toby.tobyInit(
      processName.toNativeUtf8().cast(),
      userScript.toNativeUtf8().cast(),
      ffi.Pointer.fromFunction(tobyOnLoad),
      ffi.Pointer.fromFunction(tobyOnUnload),
      ffi.Pointer.fromFunction(tobyHostCall));
}

void main() async {
  var receivePort = new ReceivePort();
  await Isolate.spawn(_toby, receivePort.sendPort);

  for (int i = 0; i < 3; i++) {
    sleep(Duration(seconds: 1));
    toby.tobyJSEmit(
        'test'.toNativeUtf8().cast(), i.toString().toNativeUtf8().cast());
  }
}
