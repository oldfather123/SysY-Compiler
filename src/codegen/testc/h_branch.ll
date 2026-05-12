; ModuleID = 'h_branch.c'
source_filename = "h_branch.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  store i32 1, ptr %2, align 4
  store i32 2, ptr %3, align 4
  store i32 3, ptr %4, align 4
  store i32 4, ptr %5, align 4
  store i32 5, ptr %6, align 4
  store i32 6, ptr %7, align 4
  %8 = load i32, ptr %2, align 4
  %9 = load i32, ptr %3, align 4
  %10 = mul nsw i32 %8, %9
  %11 = load i32, ptr %4, align 4
  %12 = add nsw i32 %10, %11
  %13 = icmp slt i32 %12, 6
  br i1 %13, label %14, label %81

14:                                               ; preds = %0
  %15 = load i32, ptr %5, align 4
  %16 = icmp ne i32 %15, 0
  br i1 %16, label %17, label %81

17:                                               ; preds = %14
  %18 = load i32, ptr %6, align 4
  %19 = icmp ne i32 %18, 0
  br i1 %19, label %27, label %20

20:                                               ; preds = %17
  %21 = load i32, ptr %2, align 4
  %22 = icmp ne i32 %21, 0
  %23 = xor i1 %22, true
  %24 = zext i1 %23 to i32
  %25 = add nsw i32 %24, 0
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %27, label %80

27:                                               ; preds = %20, %17
  %28 = load i32, ptr %4, align 4
  %29 = icmp eq i32 %28, 2
  br i1 %29, label %30, label %36

30:                                               ; preds = %27
  %31 = load i32, ptr %5, align 4
  %32 = load i32, ptr %6, align 4
  %33 = add nsw i32 %31, %32
  %34 = icmp sgt i32 %33, 2
  br i1 %34, label %35, label %36

35:                                               ; preds = %30
  store i32 3, ptr %1, align 4
  br label %82

36:                                               ; preds = %30, %27
  %37 = load i32, ptr %7, align 4
  %38 = load i32, ptr %4, align 4
  %39 = srem i32 %37, %38
  %40 = icmp ne i32 %39, 0
  br i1 %40, label %41, label %45

41:                                               ; preds = %36
  %42 = load i32, ptr %6, align 4
  %43 = icmp ne i32 %42, 0
  br i1 %43, label %44, label %45

44:                                               ; preds = %41
  store i32 4, ptr %1, align 4
  br label %82

45:                                               ; preds = %41, %36
  %46 = load i32, ptr %5, align 4
  %47 = load i32, ptr %3, align 4
  %48 = sdiv i32 %46, %47
  %49 = load i32, ptr %2, align 4
  %50 = add nsw i32 %48, %49
  %51 = icmp sge i32 %50, 2
  br i1 %51, label %52, label %79

52:                                               ; preds = %45
  %53 = load i32, ptr %6, align 4
  %54 = load i32, ptr %7, align 4
  %55 = sub nsw i32 %53, %54
  %56 = icmp sge i32 %55, 0
  br i1 %56, label %60, label %57

57:                                               ; preds = %52
  %58 = load i32, ptr %5, align 4
  %59 = icmp sgt i32 %58, 4
  br i1 %59, label %60, label %61

60:                                               ; preds = %57, %52
  store i32 6, ptr %1, align 4
  br label %82

61:                                               ; preds = %57
  %62 = load i32, ptr %4, align 4
  %63 = load i32, ptr %7, align 4
  %64 = icmp ne i32 %62, %63
  br i1 %64, label %65, label %78

65:                                               ; preds = %61
  %66 = load i32, ptr %3, align 4
  %67 = load i32, ptr %6, align 4
  %68 = load i32, ptr %5, align 4
  %69 = mul nsw i32 %67, %68
  %70 = add nsw i32 %66, %69
  %71 = icmp sgt i32 %70, 10
  br i1 %71, label %72, label %77

72:                                               ; preds = %65
  %73 = load i32, ptr %7, align 4
  %74 = icmp ne i32 %73, 0
  br i1 %74, label %76, label %75

75:                                               ; preds = %72
  store i32 9, ptr %1, align 4
  br label %82

76:                                               ; preds = %72
  store i32 10, ptr %1, align 4
  br label %82

77:                                               ; preds = %65
  store i32 8, ptr %1, align 4
  br label %82

78:                                               ; preds = %61
  store i32 7, ptr %1, align 4
  br label %82

79:                                               ; preds = %45
  store i32 5, ptr %1, align 4
  br label %82

80:                                               ; preds = %20
  store i32 2, ptr %1, align 4
  br label %82

81:                                               ; preds = %14, %0
  store i32 1, ptr %1, align 4
  br label %82

82:                                               ; preds = %81, %80, %79, %78, %77, %76, %75, %60, %44, %35
  %83 = load i32, ptr %1, align 4
  ret i32 %83
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
