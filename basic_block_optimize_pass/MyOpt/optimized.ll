; ModuleID = 'unoptimized.ll'
source_filename = "opt_tests.c"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc19.33.0"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_strength() #0 {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  %f = alloca i32, align 4
  store i32 3, ptr %a, align 4
  store i32 4, ptr %b, align 4
  %0 = load i32, ptr %c, align 4
  %shl = shl i32 %0, 1
  store i32 %shl, ptr %d, align 4
  %1 = load i32, ptr %f, align 4
  %shl1 = shl i32 %1, 3
  store i32 %shl1, ptr %e, align 4
  ret i32 3
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @compute(i32 noundef %a, i32 noundef %b) #0 {
entry:
  %b.addr = alloca i32, align 4
  %a.addr = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 %b, ptr %b.addr, align 4
  store i32 %a, ptr %a.addr, align 4
  store i32 1, ptr %result, align 4
  store i32 0, ptr %result, align 4
  store i32 23, ptr %result, align 4
  ret i32 23
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_copy() #0 {
entry:
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  store i32 4, ptr %b, align 4
  %0 = load i32, ptr %d, align 4
  store i32 %0, ptr %c, align 4
  %add = add i32 %0, 4
  store i32 %add, ptr %e, align 4
  ret i32 %add
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @compute_fold() #0 {
entry:
  %result = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 0, ptr %result, align 4
  store i32 2, ptr %a, align 4
  store i32 3, ptr %b, align 4
  store i32 9, ptr %c, align 4
  store i32 2, ptr %result, align 4
  store i32 5, ptr %result, align 4
  store i32 45, ptr %result, align 4
  store i32 22, ptr %result, align 4
  store i32 22, ptr %a, align 4
  ret i32 22
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_redundant() #0 {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 3, ptr %a, align 4
  store i32 4, ptr %b, align 4
  store i32 7, ptr %c, align 4
  ret i32 3
}

attributes #0 = { noinline nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 23.1.3 (https://github.com/llvm/llvm-project 0d261d1ca552c95a8f007e061c787ac7132fbcbc)", isOptimized: false, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "opt_tests.c", directory: "C:\\Users\\dkirthik\\Downloads\\MyOpt")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 8, !"PIC Level", i32 2}
!4 = !{i32 7, !"uwtable", i32 2}
!5 = !{i32 1, !"MaxTLSAlign", i32 65536}
!6 = !{!"clang version 23.1.3 (https://github.com/llvm/llvm-project 0d261d1ca552c95a8f007e061c787ac7132fbcbc)"}
