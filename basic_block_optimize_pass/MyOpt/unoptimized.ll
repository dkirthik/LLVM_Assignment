; ModuleID = 'opt_tests.c'
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
  %mul = mul i32 2, %0
  store i32 %mul, ptr %d, align 4
  %1 = load i32, ptr %f, align 4
  %mul1 = mul i32 %1, 8
  store i32 %mul1, ptr %e, align 4
  %2 = load i32, ptr %a, align 4
  ret i32 %2
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @compute(i32 noundef %a, i32 noundef %b) #0 {
entry:
  %b.addr = alloca i32, align 4
  %a.addr = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 %b, ptr %b.addr, align 4
  store i32 %a, ptr %a.addr, align 4
  %0 = load i32, ptr %a.addr, align 4
  %1 = load i32, ptr %a.addr, align 4
  %div = sdiv i32 %0, %1
  store i32 %div, ptr %result, align 4
  %2 = load i32, ptr %b.addr, align 4
  %3 = load i32, ptr %b.addr, align 4
  %div1 = sdiv i32 %2, %3
  %4 = load i32, ptr %result, align 4
  %mul = mul i32 %4, %div1
  store i32 %mul, ptr %result, align 4
  %5 = load i32, ptr %b.addr, align 4
  %6 = load i32, ptr %b.addr, align 4
  %sub = sub i32 %5, %6
  %7 = load i32, ptr %result, align 4
  %add = add i32 %7, %sub
  store i32 %add, ptr %result, align 4
  %8 = load i32, ptr %result, align 4
  %9 = load i32, ptr %result, align 4
  %div2 = sdiv i32 %9, %8
  store i32 %div2, ptr %result, align 4
  %10 = load i32, ptr %result, align 4
  %11 = load i32, ptr %result, align 4
  %sub3 = sub i32 %11, %10
  store i32 %sub3, ptr %result, align 4
  %12 = load i32, ptr %result, align 4
  %add4 = add i32 %12, 23
  store i32 %add4, ptr %result, align 4
  %13 = load i32, ptr %result, align 4
  ret i32 %13
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
  %1 = load i32, ptr %c, align 4
  %2 = load i32, ptr %b, align 4
  %add = add i32 %1, %2
  store i32 %add, ptr %e, align 4
  %3 = load i32, ptr %e, align 4
  ret i32 %3
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
  %0 = load i32, ptr %a, align 4
  %add = add i32 4, %0
  %1 = load i32, ptr %b, align 4
  %add1 = add i32 %add, %1
  store i32 %add1, ptr %c, align 4
  %2 = load i32, ptr %a, align 4
  %3 = load i32, ptr %result, align 4
  %add2 = add i32 %3, %2
  store i32 %add2, ptr %result, align 4
  %4 = load i32, ptr %b, align 4
  %5 = load i32, ptr %result, align 4
  %add3 = add i32 %5, %4
  store i32 %add3, ptr %result, align 4
  %6 = load i32, ptr %c, align 4
  %7 = load i32, ptr %result, align 4
  %mul = mul i32 %7, %6
  store i32 %mul, ptr %result, align 4
  %8 = load i32, ptr %result, align 4
  %div = sdiv i32 %8, 2
  store i32 %div, ptr %result, align 4
  %9 = load i32, ptr %result, align 4
  store i32 %9, ptr %a, align 4
  %10 = load i32, ptr %result, align 4
  ret i32 %10
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_redundant() #0 {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  store i32 3, ptr %a, align 4
  store i32 4, ptr %b, align 4
  store i32 3, ptr %a, align 4
  store i32 4, ptr %b, align 4
  %0 = load i32, ptr %a, align 4
  %1 = load i32, ptr %b, align 4
  %add = add i32 %0, %1
  store i32 %add, ptr %c, align 4
  %2 = load i32, ptr %a, align 4
  ret i32 %2
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
