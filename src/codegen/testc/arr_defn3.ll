; ModuleID = 'arr_defn3.c'
source_filename = "arr_defn3.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@__const.main.b = private unnamed_addr constant [4 x [2 x i32]] [[2 x i32] [i32 1, i32 2], [2 x i32] [i32 3, i32 4], [2 x i32] [i32 5, i32 6], [2 x i32] [i32 7, i32 8]], align 16
@__const.main.c = private unnamed_addr constant [4 x [2 x i32]] [[2 x i32] [i32 1, i32 2], [2 x i32] [i32 3, i32 4], [2 x i32] [i32 5, i32 6], [2 x i32] [i32 7, i32 8]], align 16
@__const.main.d = private unnamed_addr constant [4 x [2 x i32]] [[2 x i32] [i32 1, i32 2], [2 x i32] [i32 3, i32 0], [2 x i32] [i32 5, i32 0], [2 x i32] [i32 7, i32 8]], align 16

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [4 x [2 x i32]], align 16
  %3 = alloca [4 x [2 x i32]], align 16
  %4 = alloca [4 x [2 x i32]], align 16
  %5 = alloca [4 x [2 x i32]], align 16
  %6 = alloca [4 x [2 x i32]], align 16
  store i32 0, ptr %1, align 4
  call void @llvm.memset.p0.i64(ptr align 16 %2, i8 0, i64 32, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %3, ptr align 16 @__const.main.b, i64 32, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %4, ptr align 16 @__const.main.c, i64 32, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %5, ptr align 16 @__const.main.d, i64 32, i1 false)
  %7 = getelementptr inbounds [4 x [2 x i32]], ptr %6, i64 0, i64 0
  %8 = getelementptr inbounds [2 x i32], ptr %7, i64 0, i64 0
  %9 = getelementptr inbounds [4 x [2 x i32]], ptr %5, i64 0, i64 2
  %10 = getelementptr inbounds [2 x i32], ptr %9, i64 0, i64 1
  %11 = load i32, ptr %10, align 4
  store i32 %11, ptr %8, align 4
  %12 = getelementptr inbounds i32, ptr %8, i64 1
  %13 = getelementptr inbounds [4 x [2 x i32]], ptr %4, i64 0, i64 2
  %14 = getelementptr inbounds [2 x i32], ptr %13, i64 0, i64 1
  %15 = load i32, ptr %14, align 4
  store i32 %15, ptr %12, align 4
  %16 = getelementptr inbounds [2 x i32], ptr %7, i64 1
  %17 = getelementptr inbounds [2 x i32], ptr %16, i64 0, i64 0
  store i32 3, ptr %17, align 4
  %18 = getelementptr inbounds i32, ptr %17, i64 1
  store i32 4, ptr %18, align 4
  %19 = getelementptr inbounds [2 x i32], ptr %16, i64 1
  %20 = getelementptr inbounds [2 x i32], ptr %19, i64 0, i64 0
  store i32 5, ptr %20, align 4
  %21 = getelementptr inbounds i32, ptr %20, i64 1
  store i32 6, ptr %21, align 4
  %22 = getelementptr inbounds [2 x i32], ptr %19, i64 1
  %23 = getelementptr inbounds [2 x i32], ptr %22, i64 0, i64 0
  store i32 7, ptr %23, align 4
  %24 = getelementptr inbounds i32, ptr %23, i64 1
  store i32 8, ptr %24, align 4
  %25 = getelementptr inbounds [4 x [2 x i32]], ptr %6, i64 0, i64 3
  %26 = getelementptr inbounds [2 x i32], ptr %25, i64 0, i64 1
  %27 = load i32, ptr %26, align 4
  %28 = getelementptr inbounds [4 x [2 x i32]], ptr %6, i64 0, i64 0
  %29 = getelementptr inbounds [2 x i32], ptr %28, i64 0, i64 0
  %30 = load i32, ptr %29, align 16
  %31 = add nsw i32 %27, %30
  %32 = getelementptr inbounds [4 x [2 x i32]], ptr %6, i64 0, i64 0
  %33 = getelementptr inbounds [2 x i32], ptr %32, i64 0, i64 1
  %34 = load i32, ptr %33, align 4
  %35 = add nsw i32 %31, %34
  %36 = getelementptr inbounds [4 x [2 x i32]], ptr %2, i64 0, i64 2
  %37 = getelementptr inbounds [2 x i32], ptr %36, i64 0, i64 0
  %38 = load i32, ptr %37, align 16
  %39 = add nsw i32 %35, %38
  ret i32 %39
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #1

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #2

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
