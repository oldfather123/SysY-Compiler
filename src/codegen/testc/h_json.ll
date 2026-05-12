; ModuleID = 'h_json.c'
source_filename = "h_json.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@pos = dso_local global i32 0, align 4
@__const.detect_item.mTrue = private unnamed_addr constant [4 x i32] [i32 116, i32 114, i32 117, i32 101], align 16
@__const.detect_item.mFalse = private unnamed_addr constant [5 x i32] [i32 102, i32 97, i32 108, i32 115, i32 101], align 16
@__const.detect_item.mNull = private unnamed_addr constant [4 x i32] [i32 110, i32 117, i32 108, i32 108], align 16
@buffer = dso_local global [50000000 x i32] zeroinitializer, align 16

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @is_number(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = load i32, ptr %3, align 4
  %5 = icmp sge i32 %4, 48
  br i1 %5, label %6, label %11

6:                                                ; preds = %1
  %7 = load i32, ptr %3, align 4
  %8 = icmp sle i32 %7, 57
  br i1 %8, label %9, label %10

9:                                                ; preds = %6
  store i32 1, ptr %2, align 4
  br label %12

10:                                               ; preds = %6
  store i32 0, ptr %2, align 4
  br label %12

11:                                               ; preds = %1
  store i32 0, ptr %2, align 4
  br label %12

12:                                               ; preds = %11, %10, %9
  %13 = load i32, ptr %2, align 4
  ret i32 %13
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @skip_space(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  br label %5

5:                                                ; preds = %2, %54
  %6 = load i32, ptr @pos, align 4
  %7 = load i32, ptr %4, align 4
  %8 = icmp sge i32 %6, %7
  br i1 %8, label %9, label %10

9:                                                ; preds = %5
  br label %55

10:                                               ; preds = %5
  %11 = load ptr, ptr %3, align 8
  %12 = load i32, ptr @pos, align 4
  %13 = sext i32 %12 to i64
  %14 = getelementptr inbounds i32, ptr %11, i64 %13
  %15 = load i32, ptr %14, align 4
  %16 = icmp eq i32 %15, 32
  br i1 %16, label %17, label %20

17:                                               ; preds = %10
  %18 = load i32, ptr @pos, align 4
  %19 = add nsw i32 %18, 1
  store i32 %19, ptr @pos, align 4
  br label %54

20:                                               ; preds = %10
  %21 = load ptr, ptr %3, align 8
  %22 = load i32, ptr @pos, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds i32, ptr %21, i64 %23
  %25 = load i32, ptr %24, align 4
  %26 = icmp eq i32 %25, 9
  br i1 %26, label %27, label %30

27:                                               ; preds = %20
  %28 = load i32, ptr @pos, align 4
  %29 = add nsw i32 %28, 1
  store i32 %29, ptr @pos, align 4
  br label %53

30:                                               ; preds = %20
  %31 = load ptr, ptr %3, align 8
  %32 = load i32, ptr @pos, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds i32, ptr %31, i64 %33
  %35 = load i32, ptr %34, align 4
  %36 = icmp eq i32 %35, 10
  br i1 %36, label %37, label %40

37:                                               ; preds = %30
  %38 = load i32, ptr @pos, align 4
  %39 = add nsw i32 %38, 1
  store i32 %39, ptr @pos, align 4
  br label %52

40:                                               ; preds = %30
  %41 = load ptr, ptr %3, align 8
  %42 = load i32, ptr @pos, align 4
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds i32, ptr %41, i64 %43
  %45 = load i32, ptr %44, align 4
  %46 = icmp eq i32 %45, 13
  br i1 %46, label %47, label %50

47:                                               ; preds = %40
  %48 = load i32, ptr @pos, align 4
  %49 = add nsw i32 %48, 1
  store i32 %49, ptr @pos, align 4
  br label %51

50:                                               ; preds = %40
  br label %55

51:                                               ; preds = %47
  br label %52

52:                                               ; preds = %51, %37
  br label %53

53:                                               ; preds = %52, %27
  br label %54

54:                                               ; preds = %53, %17
  br label %5

55:                                               ; preds = %50, %9
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @detect_item(i32 noundef %0, ptr noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca ptr, align 8
  %7 = alloca i32, align 4
  %8 = alloca [4 x i32], align 16
  %9 = alloca [5 x i32], align 16
  %10 = alloca [4 x i32], align 16
  store i32 %0, ptr %5, align 4
  store ptr %1, ptr %6, align 8
  store i32 %2, ptr %7, align 4
  %11 = load i32, ptr @pos, align 4
  %12 = load i32, ptr %7, align 4
  %13 = icmp sge i32 %11, %12
  br i1 %13, label %14, label %15

14:                                               ; preds = %3
  store i32 0, ptr %4, align 4
  br label %708

15:                                               ; preds = %3
  %16 = load ptr, ptr %6, align 8
  %17 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %16, i32 noundef %17)
  %18 = load i32, ptr %5, align 4
  %19 = icmp eq i32 %18, 0
  br i1 %19, label %20, label %121

20:                                               ; preds = %15
  %21 = load ptr, ptr %6, align 8
  %22 = load i32, ptr @pos, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds i32, ptr %21, i64 %23
  %25 = load i32, ptr %24, align 4
  %26 = icmp eq i32 %25, 123
  br i1 %26, label %27, label %31

27:                                               ; preds = %20
  %28 = load ptr, ptr %6, align 8
  %29 = load i32, ptr %7, align 4
  %30 = call i32 @detect_item(i32 noundef 4, ptr noundef %28, i32 noundef %29)
  store i32 %30, ptr %4, align 4
  br label %708

31:                                               ; preds = %20
  %32 = load ptr, ptr %6, align 8
  %33 = load i32, ptr @pos, align 4
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds i32, ptr %32, i64 %34
  %36 = load i32, ptr %35, align 4
  %37 = icmp eq i32 %36, 91
  br i1 %37, label %38, label %42

38:                                               ; preds = %31
  %39 = load ptr, ptr %6, align 8
  %40 = load i32, ptr %7, align 4
  %41 = call i32 @detect_item(i32 noundef 3, ptr noundef %39, i32 noundef %40)
  store i32 %41, ptr %4, align 4
  br label %708

42:                                               ; preds = %31
  %43 = load ptr, ptr %6, align 8
  %44 = load i32, ptr @pos, align 4
  %45 = sext i32 %44 to i64
  %46 = getelementptr inbounds i32, ptr %43, i64 %45
  %47 = load i32, ptr %46, align 4
  %48 = icmp eq i32 %47, 34
  br i1 %48, label %49, label %53

49:                                               ; preds = %42
  %50 = load ptr, ptr %6, align 8
  %51 = load i32, ptr %7, align 4
  %52 = call i32 @detect_item(i32 noundef 2, ptr noundef %50, i32 noundef %51)
  store i32 %52, ptr %4, align 4
  br label %708

53:                                               ; preds = %42
  %54 = load ptr, ptr %6, align 8
  %55 = load i32, ptr @pos, align 4
  %56 = sext i32 %55 to i64
  %57 = getelementptr inbounds i32, ptr %54, i64 %56
  %58 = load i32, ptr %57, align 4
  %59 = call i32 @is_number(i32 noundef %58)
  %60 = icmp eq i32 %59, 1
  br i1 %60, label %61, label %65

61:                                               ; preds = %53
  %62 = load ptr, ptr %6, align 8
  %63 = load i32, ptr %7, align 4
  %64 = call i32 @detect_item(i32 noundef 1, ptr noundef %62, i32 noundef %63)
  store i32 %64, ptr %4, align 4
  br label %708

65:                                               ; preds = %53
  %66 = load ptr, ptr %6, align 8
  %67 = load i32, ptr @pos, align 4
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds i32, ptr %66, i64 %68
  %70 = load i32, ptr %69, align 4
  %71 = icmp eq i32 %70, 43
  br i1 %71, label %72, label %76

72:                                               ; preds = %65
  %73 = load ptr, ptr %6, align 8
  %74 = load i32, ptr %7, align 4
  %75 = call i32 @detect_item(i32 noundef 1, ptr noundef %73, i32 noundef %74)
  store i32 %75, ptr %4, align 4
  br label %708

76:                                               ; preds = %65
  %77 = load ptr, ptr %6, align 8
  %78 = load i32, ptr @pos, align 4
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds i32, ptr %77, i64 %79
  %81 = load i32, ptr %80, align 4
  %82 = icmp eq i32 %81, 45
  br i1 %82, label %83, label %87

83:                                               ; preds = %76
  %84 = load ptr, ptr %6, align 8
  %85 = load i32, ptr %7, align 4
  %86 = call i32 @detect_item(i32 noundef 1, ptr noundef %84, i32 noundef %85)
  store i32 %86, ptr %4, align 4
  br label %708

87:                                               ; preds = %76
  %88 = load ptr, ptr %6, align 8
  %89 = load i32, ptr @pos, align 4
  %90 = sext i32 %89 to i64
  %91 = getelementptr inbounds i32, ptr %88, i64 %90
  %92 = load i32, ptr %91, align 4
  %93 = icmp eq i32 %92, 116
  br i1 %93, label %94, label %98

94:                                               ; preds = %87
  %95 = load ptr, ptr %6, align 8
  %96 = load i32, ptr %7, align 4
  %97 = call i32 @detect_item(i32 noundef 5, ptr noundef %95, i32 noundef %96)
  store i32 %97, ptr %4, align 4
  br label %708

98:                                               ; preds = %87
  %99 = load ptr, ptr %6, align 8
  %100 = load i32, ptr @pos, align 4
  %101 = sext i32 %100 to i64
  %102 = getelementptr inbounds i32, ptr %99, i64 %101
  %103 = load i32, ptr %102, align 4
  %104 = icmp eq i32 %103, 102
  br i1 %104, label %105, label %109

105:                                              ; preds = %98
  %106 = load ptr, ptr %6, align 8
  %107 = load i32, ptr %7, align 4
  %108 = call i32 @detect_item(i32 noundef 6, ptr noundef %106, i32 noundef %107)
  store i32 %108, ptr %4, align 4
  br label %708

109:                                              ; preds = %98
  %110 = load ptr, ptr %6, align 8
  %111 = load i32, ptr @pos, align 4
  %112 = sext i32 %111 to i64
  %113 = getelementptr inbounds i32, ptr %110, i64 %112
  %114 = load i32, ptr %113, align 4
  %115 = icmp eq i32 %114, 110
  br i1 %115, label %116, label %120

116:                                              ; preds = %109
  %117 = load ptr, ptr %6, align 8
  %118 = load i32, ptr %7, align 4
  %119 = call i32 @detect_item(i32 noundef 7, ptr noundef %117, i32 noundef %118)
  store i32 %119, ptr %4, align 4
  br label %708

120:                                              ; preds = %109
  store i32 0, ptr %4, align 4
  br label %708

121:                                              ; preds = %15
  %122 = load i32, ptr %5, align 4
  %123 = icmp eq i32 %122, 1
  br i1 %123, label %124, label %272

124:                                              ; preds = %121
  %125 = load ptr, ptr %6, align 8
  %126 = load i32, ptr @pos, align 4
  %127 = sext i32 %126 to i64
  %128 = getelementptr inbounds i32, ptr %125, i64 %127
  %129 = load i32, ptr %128, align 4
  %130 = icmp eq i32 %129, 43
  br i1 %130, label %131, label %134

131:                                              ; preds = %124
  %132 = load i32, ptr @pos, align 4
  %133 = add nsw i32 %132, 1
  store i32 %133, ptr @pos, align 4
  br label %145

134:                                              ; preds = %124
  %135 = load ptr, ptr %6, align 8
  %136 = load i32, ptr @pos, align 4
  %137 = sext i32 %136 to i64
  %138 = getelementptr inbounds i32, ptr %135, i64 %137
  %139 = load i32, ptr %138, align 4
  %140 = icmp eq i32 %139, 45
  br i1 %140, label %141, label %144

141:                                              ; preds = %134
  %142 = load i32, ptr @pos, align 4
  %143 = add nsw i32 %142, 1
  store i32 %143, ptr @pos, align 4
  br label %144

144:                                              ; preds = %141, %134
  br label %145

145:                                              ; preds = %144, %131
  %146 = load i32, ptr @pos, align 4
  %147 = load i32, ptr %7, align 4
  %148 = icmp sge i32 %146, %147
  br i1 %148, label %149, label %150

149:                                              ; preds = %145
  store i32 0, ptr %4, align 4
  br label %708

150:                                              ; preds = %145
  %151 = load ptr, ptr %6, align 8
  %152 = load i32, ptr @pos, align 4
  %153 = sext i32 %152 to i64
  %154 = getelementptr inbounds i32, ptr %151, i64 %153
  %155 = load i32, ptr %154, align 4
  %156 = call i32 @is_number(i32 noundef %155)
  %157 = icmp eq i32 %156, 0
  br i1 %157, label %158, label %159

158:                                              ; preds = %150
  store i32 0, ptr %4, align 4
  br label %708

159:                                              ; preds = %150
  br label %160

160:                                              ; preds = %159
  br label %161

161:                                              ; preds = %174, %160
  %162 = load i32, ptr @pos, align 4
  %163 = load i32, ptr %7, align 4
  %164 = icmp slt i32 %162, %163
  br i1 %164, label %165, label %177

165:                                              ; preds = %161
  %166 = load ptr, ptr %6, align 8
  %167 = load i32, ptr @pos, align 4
  %168 = sext i32 %167 to i64
  %169 = getelementptr inbounds i32, ptr %166, i64 %168
  %170 = load i32, ptr %169, align 4
  %171 = call i32 @is_number(i32 noundef %170)
  %172 = icmp ne i32 %171, 1
  br i1 %172, label %173, label %174

173:                                              ; preds = %165
  br label %177

174:                                              ; preds = %165
  %175 = load i32, ptr @pos, align 4
  %176 = add nsw i32 %175, 1
  store i32 %176, ptr @pos, align 4
  br label %161, !llvm.loop !6

177:                                              ; preds = %173, %161
  %178 = load i32, ptr @pos, align 4
  %179 = load i32, ptr %7, align 4
  %180 = icmp slt i32 %178, %179
  br i1 %180, label %181, label %209

181:                                              ; preds = %177
  %182 = load ptr, ptr %6, align 8
  %183 = load i32, ptr @pos, align 4
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds i32, ptr %182, i64 %184
  %186 = load i32, ptr %185, align 4
  %187 = icmp eq i32 %186, 46
  br i1 %187, label %188, label %208

188:                                              ; preds = %181
  %189 = load i32, ptr @pos, align 4
  %190 = add nsw i32 %189, 1
  store i32 %190, ptr @pos, align 4
  br label %191

191:                                              ; preds = %204, %188
  %192 = load i32, ptr @pos, align 4
  %193 = load i32, ptr %7, align 4
  %194 = icmp slt i32 %192, %193
  br i1 %194, label %195, label %207

195:                                              ; preds = %191
  %196 = load ptr, ptr %6, align 8
  %197 = load i32, ptr @pos, align 4
  %198 = sext i32 %197 to i64
  %199 = getelementptr inbounds i32, ptr %196, i64 %198
  %200 = load i32, ptr %199, align 4
  %201 = call i32 @is_number(i32 noundef %200)
  %202 = icmp ne i32 %201, 1
  br i1 %202, label %203, label %204

203:                                              ; preds = %195
  br label %207

204:                                              ; preds = %195
  %205 = load i32, ptr @pos, align 4
  %206 = add nsw i32 %205, 1
  store i32 %206, ptr @pos, align 4
  br label %191, !llvm.loop !8

207:                                              ; preds = %203, %191
  br label %208

208:                                              ; preds = %207, %181
  br label %209

209:                                              ; preds = %208, %177
  %210 = load i32, ptr @pos, align 4
  %211 = load i32, ptr %7, align 4
  %212 = icmp slt i32 %210, %211
  br i1 %212, label %213, label %271

213:                                              ; preds = %209
  %214 = load ptr, ptr %6, align 8
  %215 = load i32, ptr @pos, align 4
  %216 = sext i32 %215 to i64
  %217 = getelementptr inbounds i32, ptr %214, i64 %216
  %218 = load i32, ptr %217, align 4
  %219 = icmp eq i32 %218, 101
  br i1 %219, label %220, label %270

220:                                              ; preds = %213
  %221 = load i32, ptr @pos, align 4
  %222 = add nsw i32 %221, 1
  store i32 %222, ptr @pos, align 4
  %223 = load i32, ptr @pos, align 4
  %224 = load i32, ptr %7, align 4
  %225 = icmp slt i32 %223, %224
  br i1 %225, label %226, label %237

226:                                              ; preds = %220
  %227 = load ptr, ptr %6, align 8
  %228 = load i32, ptr @pos, align 4
  %229 = sext i32 %228 to i64
  %230 = getelementptr inbounds i32, ptr %227, i64 %229
  %231 = load i32, ptr %230, align 4
  %232 = icmp eq i32 %231, 43
  br i1 %232, label %233, label %236

233:                                              ; preds = %226
  %234 = load i32, ptr @pos, align 4
  %235 = add nsw i32 %234, 1
  store i32 %235, ptr @pos, align 4
  br label %236

236:                                              ; preds = %233, %226
  br label %237

237:                                              ; preds = %236, %220
  %238 = load i32, ptr @pos, align 4
  %239 = load i32, ptr %7, align 4
  %240 = icmp slt i32 %238, %239
  br i1 %240, label %241, label %252

241:                                              ; preds = %237
  %242 = load ptr, ptr %6, align 8
  %243 = load i32, ptr @pos, align 4
  %244 = sext i32 %243 to i64
  %245 = getelementptr inbounds i32, ptr %242, i64 %244
  %246 = load i32, ptr %245, align 4
  %247 = icmp eq i32 %246, 45
  br i1 %247, label %248, label %251

248:                                              ; preds = %241
  %249 = load i32, ptr @pos, align 4
  %250 = add nsw i32 %249, 1
  store i32 %250, ptr @pos, align 4
  br label %251

251:                                              ; preds = %248, %241
  br label %252

252:                                              ; preds = %251, %237
  br label %253

253:                                              ; preds = %266, %252
  %254 = load i32, ptr @pos, align 4
  %255 = load i32, ptr %7, align 4
  %256 = icmp slt i32 %254, %255
  br i1 %256, label %257, label %269

257:                                              ; preds = %253
  %258 = load ptr, ptr %6, align 8
  %259 = load i32, ptr @pos, align 4
  %260 = sext i32 %259 to i64
  %261 = getelementptr inbounds i32, ptr %258, i64 %260
  %262 = load i32, ptr %261, align 4
  %263 = call i32 @is_number(i32 noundef %262)
  %264 = icmp ne i32 %263, 1
  br i1 %264, label %265, label %266

265:                                              ; preds = %257
  br label %269

266:                                              ; preds = %257
  %267 = load i32, ptr @pos, align 4
  %268 = add nsw i32 %267, 1
  store i32 %268, ptr @pos, align 4
  br label %253, !llvm.loop !9

269:                                              ; preds = %265, %253
  br label %270

270:                                              ; preds = %269, %213
  br label %271

271:                                              ; preds = %270, %209
  br label %706

272:                                              ; preds = %121
  %273 = load i32, ptr %5, align 4
  %274 = icmp eq i32 %273, 2
  br i1 %274, label %275, label %321

275:                                              ; preds = %272
  %276 = load i32, ptr @pos, align 4
  %277 = add nsw i32 %276, 1
  store i32 %277, ptr @pos, align 4
  br label %278

278:                                              ; preds = %303, %275
  %279 = load i32, ptr @pos, align 4
  %280 = load i32, ptr %7, align 4
  %281 = icmp slt i32 %279, %280
  br i1 %281, label %282, label %304

282:                                              ; preds = %278
  %283 = load ptr, ptr %6, align 8
  %284 = load i32, ptr @pos, align 4
  %285 = sext i32 %284 to i64
  %286 = getelementptr inbounds i32, ptr %283, i64 %285
  %287 = load i32, ptr %286, align 4
  %288 = icmp eq i32 %287, 34
  br i1 %288, label %289, label %290

289:                                              ; preds = %282
  br label %304

290:                                              ; preds = %282
  %291 = load ptr, ptr %6, align 8
  %292 = load i32, ptr @pos, align 4
  %293 = sext i32 %292 to i64
  %294 = getelementptr inbounds i32, ptr %291, i64 %293
  %295 = load i32, ptr %294, align 4
  %296 = icmp eq i32 %295, 92
  br i1 %296, label %297, label %300

297:                                              ; preds = %290
  %298 = load i32, ptr @pos, align 4
  %299 = add nsw i32 %298, 2
  store i32 %299, ptr @pos, align 4
  br label %303

300:                                              ; preds = %290
  %301 = load i32, ptr @pos, align 4
  %302 = add nsw i32 %301, 1
  store i32 %302, ptr @pos, align 4
  br label %303

303:                                              ; preds = %300, %297
  br label %278, !llvm.loop !10

304:                                              ; preds = %289, %278
  %305 = load i32, ptr @pos, align 4
  %306 = load i32, ptr %7, align 4
  %307 = icmp sge i32 %305, %306
  br i1 %307, label %308, label %309

308:                                              ; preds = %304
  store i32 0, ptr %4, align 4
  br label %708

309:                                              ; preds = %304
  %310 = load ptr, ptr %6, align 8
  %311 = load i32, ptr @pos, align 4
  %312 = sext i32 %311 to i64
  %313 = getelementptr inbounds i32, ptr %310, i64 %312
  %314 = load i32, ptr %313, align 4
  %315 = icmp ne i32 %314, 34
  br i1 %315, label %316, label %317

316:                                              ; preds = %309
  store i32 0, ptr %4, align 4
  br label %708

317:                                              ; preds = %309
  br label %318

318:                                              ; preds = %317
  %319 = load i32, ptr @pos, align 4
  %320 = add nsw i32 %319, 1
  store i32 %320, ptr @pos, align 4
  br label %705

321:                                              ; preds = %272
  %322 = load i32, ptr %5, align 4
  %323 = icmp eq i32 %322, 3
  br i1 %323, label %324, label %390

324:                                              ; preds = %321
  %325 = load i32, ptr @pos, align 4
  %326 = add nsw i32 %325, 1
  store i32 %326, ptr @pos, align 4
  %327 = load ptr, ptr %6, align 8
  %328 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %327, i32 noundef %328)
  %329 = load i32, ptr @pos, align 4
  %330 = load i32, ptr %7, align 4
  %331 = icmp slt i32 %329, %330
  br i1 %331, label %332, label %343

332:                                              ; preds = %324
  %333 = load ptr, ptr %6, align 8
  %334 = load i32, ptr @pos, align 4
  %335 = sext i32 %334 to i64
  %336 = getelementptr inbounds i32, ptr %333, i64 %335
  %337 = load i32, ptr %336, align 4
  %338 = icmp eq i32 %337, 93
  br i1 %338, label %339, label %342

339:                                              ; preds = %332
  %340 = load i32, ptr @pos, align 4
  %341 = add nsw i32 %340, 1
  store i32 %341, ptr @pos, align 4
  store i32 1, ptr %4, align 4
  br label %708

342:                                              ; preds = %332
  br label %343

343:                                              ; preds = %342, %324
  %344 = load ptr, ptr %6, align 8
  %345 = load i32, ptr %7, align 4
  %346 = call i32 @detect_item(i32 noundef 0, ptr noundef %344, i32 noundef %345)
  %347 = icmp eq i32 %346, 0
  br i1 %347, label %348, label %349

348:                                              ; preds = %343
  store i32 0, ptr %4, align 4
  br label %708

349:                                              ; preds = %343
  %350 = load ptr, ptr %6, align 8
  %351 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %350, i32 noundef %351)
  br label %352

352:                                              ; preds = %369, %349
  %353 = load ptr, ptr %6, align 8
  %354 = load i32, ptr @pos, align 4
  %355 = sext i32 %354 to i64
  %356 = getelementptr inbounds i32, ptr %353, i64 %355
  %357 = load i32, ptr %356, align 4
  %358 = icmp eq i32 %357, 44
  br i1 %358, label %359, label %372

359:                                              ; preds = %352
  %360 = load i32, ptr @pos, align 4
  %361 = add nsw i32 %360, 1
  store i32 %361, ptr @pos, align 4
  %362 = load ptr, ptr %6, align 8
  %363 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %362, i32 noundef %363)
  %364 = load ptr, ptr %6, align 8
  %365 = load i32, ptr %7, align 4
  %366 = call i32 @detect_item(i32 noundef 0, ptr noundef %364, i32 noundef %365)
  %367 = icmp eq i32 %366, 0
  br i1 %367, label %368, label %369

368:                                              ; preds = %359
  store i32 0, ptr %4, align 4
  br label %708

369:                                              ; preds = %359
  %370 = load ptr, ptr %6, align 8
  %371 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %370, i32 noundef %371)
  br label %352, !llvm.loop !11

372:                                              ; preds = %352
  %373 = load ptr, ptr %6, align 8
  %374 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %373, i32 noundef %374)
  %375 = load i32, ptr @pos, align 4
  %376 = load i32, ptr %7, align 4
  %377 = icmp sge i32 %375, %376
  br i1 %377, label %378, label %379

378:                                              ; preds = %372
  store i32 0, ptr %4, align 4
  br label %708

379:                                              ; preds = %372
  %380 = load ptr, ptr %6, align 8
  %381 = load i32, ptr @pos, align 4
  %382 = sext i32 %381 to i64
  %383 = getelementptr inbounds i32, ptr %380, i64 %382
  %384 = load i32, ptr %383, align 4
  %385 = icmp ne i32 %384, 93
  br i1 %385, label %386, label %387

386:                                              ; preds = %379
  store i32 0, ptr %4, align 4
  br label %708

387:                                              ; preds = %379
  %388 = load i32, ptr @pos, align 4
  %389 = add nsw i32 %388, 1
  store i32 %389, ptr @pos, align 4
  br label %704

390:                                              ; preds = %321
  %391 = load i32, ptr %5, align 4
  %392 = icmp eq i32 %391, 4
  br i1 %392, label %393, label %510

393:                                              ; preds = %390
  %394 = load i32, ptr @pos, align 4
  %395 = add nsw i32 %394, 1
  store i32 %395, ptr @pos, align 4
  %396 = load ptr, ptr %6, align 8
  %397 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %396, i32 noundef %397)
  %398 = load i32, ptr @pos, align 4
  %399 = load i32, ptr %7, align 4
  %400 = icmp slt i32 %398, %399
  br i1 %400, label %401, label %412

401:                                              ; preds = %393
  %402 = load ptr, ptr %6, align 8
  %403 = load i32, ptr @pos, align 4
  %404 = sext i32 %403 to i64
  %405 = getelementptr inbounds i32, ptr %402, i64 %404
  %406 = load i32, ptr %405, align 4
  %407 = icmp eq i32 %406, 125
  br i1 %407, label %408, label %411

408:                                              ; preds = %401
  %409 = load i32, ptr @pos, align 4
  %410 = add nsw i32 %409, 1
  store i32 %410, ptr @pos, align 4
  store i32 1, ptr %4, align 4
  br label %708

411:                                              ; preds = %401
  br label %412

412:                                              ; preds = %411, %393
  %413 = load ptr, ptr %6, align 8
  %414 = load i32, ptr %7, align 4
  %415 = call i32 @detect_item(i32 noundef 2, ptr noundef %413, i32 noundef %414)
  %416 = icmp eq i32 %415, 0
  br i1 %416, label %417, label %418

417:                                              ; preds = %412
  store i32 0, ptr %4, align 4
  br label %708

418:                                              ; preds = %412
  %419 = load ptr, ptr %6, align 8
  %420 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %419, i32 noundef %420)
  %421 = load i32, ptr @pos, align 4
  %422 = load i32, ptr %7, align 4
  %423 = icmp sge i32 %421, %422
  br i1 %423, label %424, label %425

424:                                              ; preds = %418
  store i32 0, ptr %4, align 4
  br label %708

425:                                              ; preds = %418
  %426 = load ptr, ptr %6, align 8
  %427 = load i32, ptr @pos, align 4
  %428 = sext i32 %427 to i64
  %429 = getelementptr inbounds i32, ptr %426, i64 %428
  %430 = load i32, ptr %429, align 4
  %431 = icmp ne i32 %430, 58
  br i1 %431, label %432, label %433

432:                                              ; preds = %425
  store i32 0, ptr %4, align 4
  br label %708

433:                                              ; preds = %425
  %434 = load i32, ptr @pos, align 4
  %435 = add nsw i32 %434, 1
  store i32 %435, ptr @pos, align 4
  %436 = load ptr, ptr %6, align 8
  %437 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %436, i32 noundef %437)
  %438 = load ptr, ptr %6, align 8
  %439 = load i32, ptr %7, align 4
  %440 = call i32 @detect_item(i32 noundef 0, ptr noundef %438, i32 noundef %439)
  %441 = icmp eq i32 %440, 0
  br i1 %441, label %442, label %443

442:                                              ; preds = %433
  store i32 0, ptr %4, align 4
  br label %708

443:                                              ; preds = %433
  %444 = load ptr, ptr %6, align 8
  %445 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %444, i32 noundef %445)
  br label %446

446:                                              ; preds = %488, %443
  %447 = load ptr, ptr %6, align 8
  %448 = load i32, ptr @pos, align 4
  %449 = sext i32 %448 to i64
  %450 = getelementptr inbounds i32, ptr %447, i64 %449
  %451 = load i32, ptr %450, align 4
  %452 = icmp eq i32 %451, 44
  br i1 %452, label %453, label %491

453:                                              ; preds = %446
  %454 = load i32, ptr @pos, align 4
  %455 = add nsw i32 %454, 1
  store i32 %455, ptr @pos, align 4
  %456 = load ptr, ptr %6, align 8
  %457 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %456, i32 noundef %457)
  %458 = load ptr, ptr %6, align 8
  %459 = load i32, ptr %7, align 4
  %460 = call i32 @detect_item(i32 noundef 2, ptr noundef %458, i32 noundef %459)
  %461 = icmp eq i32 %460, 0
  br i1 %461, label %462, label %463

462:                                              ; preds = %453
  store i32 0, ptr %4, align 4
  br label %708

463:                                              ; preds = %453
  %464 = load ptr, ptr %6, align 8
  %465 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %464, i32 noundef %465)
  %466 = load i32, ptr @pos, align 4
  %467 = load i32, ptr %7, align 4
  %468 = icmp sge i32 %466, %467
  br i1 %468, label %469, label %470

469:                                              ; preds = %463
  store i32 0, ptr %4, align 4
  br label %708

470:                                              ; preds = %463
  %471 = load ptr, ptr %6, align 8
  %472 = load i32, ptr @pos, align 4
  %473 = sext i32 %472 to i64
  %474 = getelementptr inbounds i32, ptr %471, i64 %473
  %475 = load i32, ptr %474, align 4
  %476 = icmp ne i32 %475, 58
  br i1 %476, label %477, label %478

477:                                              ; preds = %470
  store i32 0, ptr %4, align 4
  br label %708

478:                                              ; preds = %470
  %479 = load i32, ptr @pos, align 4
  %480 = add nsw i32 %479, 1
  store i32 %480, ptr @pos, align 4
  %481 = load ptr, ptr %6, align 8
  %482 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %481, i32 noundef %482)
  %483 = load ptr, ptr %6, align 8
  %484 = load i32, ptr %7, align 4
  %485 = call i32 @detect_item(i32 noundef 0, ptr noundef %483, i32 noundef %484)
  %486 = icmp eq i32 %485, 0
  br i1 %486, label %487, label %488

487:                                              ; preds = %478
  store i32 0, ptr %4, align 4
  br label %708

488:                                              ; preds = %478
  %489 = load ptr, ptr %6, align 8
  %490 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %489, i32 noundef %490)
  br label %446, !llvm.loop !12

491:                                              ; preds = %446
  %492 = load ptr, ptr %6, align 8
  %493 = load i32, ptr %7, align 4
  call void @skip_space(ptr noundef %492, i32 noundef %493)
  %494 = load i32, ptr @pos, align 4
  %495 = load i32, ptr %7, align 4
  %496 = icmp sge i32 %494, %495
  br i1 %496, label %497, label %498

497:                                              ; preds = %491
  store i32 0, ptr %4, align 4
  br label %708

498:                                              ; preds = %491
  %499 = load ptr, ptr %6, align 8
  %500 = load i32, ptr @pos, align 4
  %501 = sext i32 %500 to i64
  %502 = getelementptr inbounds i32, ptr %499, i64 %501
  %503 = load i32, ptr %502, align 4
  %504 = icmp ne i32 %503, 125
  br i1 %504, label %505, label %506

505:                                              ; preds = %498
  store i32 0, ptr %4, align 4
  br label %708

506:                                              ; preds = %498
  br label %507

507:                                              ; preds = %506
  %508 = load i32, ptr @pos, align 4
  %509 = add nsw i32 %508, 1
  store i32 %509, ptr @pos, align 4
  br label %703

510:                                              ; preds = %390
  %511 = load i32, ptr %5, align 4
  %512 = icmp eq i32 %511, 5
  br i1 %512, label %513, label %569

513:                                              ; preds = %510
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %8, ptr align 16 @__const.detect_item.mTrue, i64 16, i1 false)
  %514 = load i32, ptr @pos, align 4
  %515 = add nsw i32 %514, 3
  %516 = load i32, ptr %7, align 4
  %517 = icmp sge i32 %515, %516
  br i1 %517, label %518, label %519

518:                                              ; preds = %513
  store i32 0, ptr %4, align 4
  br label %708

519:                                              ; preds = %513
  %520 = load ptr, ptr %6, align 8
  %521 = load i32, ptr @pos, align 4
  %522 = sext i32 %521 to i64
  %523 = getelementptr inbounds i32, ptr %520, i64 %522
  %524 = load i32, ptr %523, align 4
  %525 = getelementptr inbounds [4 x i32], ptr %8, i64 0, i64 0
  %526 = load i32, ptr %525, align 16
  %527 = icmp ne i32 %524, %526
  br i1 %527, label %528, label %529

528:                                              ; preds = %519
  store i32 0, ptr %4, align 4
  br label %708

529:                                              ; preds = %519
  %530 = load ptr, ptr %6, align 8
  %531 = load i32, ptr @pos, align 4
  %532 = add nsw i32 %531, 1
  %533 = sext i32 %532 to i64
  %534 = getelementptr inbounds i32, ptr %530, i64 %533
  %535 = load i32, ptr %534, align 4
  %536 = getelementptr inbounds [4 x i32], ptr %8, i64 0, i64 1
  %537 = load i32, ptr %536, align 4
  %538 = icmp ne i32 %535, %537
  br i1 %538, label %539, label %540

539:                                              ; preds = %529
  store i32 0, ptr %4, align 4
  br label %708

540:                                              ; preds = %529
  %541 = load ptr, ptr %6, align 8
  %542 = load i32, ptr @pos, align 4
  %543 = add nsw i32 %542, 2
  %544 = sext i32 %543 to i64
  %545 = getelementptr inbounds i32, ptr %541, i64 %544
  %546 = load i32, ptr %545, align 4
  %547 = getelementptr inbounds [4 x i32], ptr %8, i64 0, i64 2
  %548 = load i32, ptr %547, align 8
  %549 = icmp ne i32 %546, %548
  br i1 %549, label %550, label %551

550:                                              ; preds = %540
  store i32 0, ptr %4, align 4
  br label %708

551:                                              ; preds = %540
  %552 = load ptr, ptr %6, align 8
  %553 = load i32, ptr @pos, align 4
  %554 = add nsw i32 %553, 3
  %555 = sext i32 %554 to i64
  %556 = getelementptr inbounds i32, ptr %552, i64 %555
  %557 = load i32, ptr %556, align 4
  %558 = getelementptr inbounds [4 x i32], ptr %8, i64 0, i64 3
  %559 = load i32, ptr %558, align 4
  %560 = icmp ne i32 %557, %559
  br i1 %560, label %561, label %562

561:                                              ; preds = %551
  store i32 0, ptr %4, align 4
  br label %708

562:                                              ; preds = %551
  br label %563

563:                                              ; preds = %562
  br label %564

564:                                              ; preds = %563
  br label %565

565:                                              ; preds = %564
  br label %566

566:                                              ; preds = %565
  %567 = load i32, ptr @pos, align 4
  %568 = add nsw i32 %567, 4
  store i32 %568, ptr @pos, align 4
  br label %702

569:                                              ; preds = %510
  %570 = load i32, ptr %5, align 4
  %571 = icmp eq i32 %570, 6
  br i1 %571, label %572, label %640

572:                                              ; preds = %569
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %9, ptr align 16 @__const.detect_item.mFalse, i64 20, i1 false)
  %573 = load i32, ptr @pos, align 4
  %574 = add nsw i32 %573, 4
  %575 = load i32, ptr %7, align 4
  %576 = icmp sge i32 %574, %575
  br i1 %576, label %577, label %578

577:                                              ; preds = %572
  store i32 0, ptr %4, align 4
  br label %708

578:                                              ; preds = %572
  %579 = load ptr, ptr %6, align 8
  %580 = load i32, ptr @pos, align 4
  %581 = sext i32 %580 to i64
  %582 = getelementptr inbounds i32, ptr %579, i64 %581
  %583 = load i32, ptr %582, align 4
  %584 = getelementptr inbounds [5 x i32], ptr %9, i64 0, i64 0
  %585 = load i32, ptr %584, align 16
  %586 = icmp ne i32 %583, %585
  br i1 %586, label %587, label %588

587:                                              ; preds = %578
  store i32 0, ptr %4, align 4
  br label %708

588:                                              ; preds = %578
  %589 = load ptr, ptr %6, align 8
  %590 = load i32, ptr @pos, align 4
  %591 = add nsw i32 %590, 1
  %592 = sext i32 %591 to i64
  %593 = getelementptr inbounds i32, ptr %589, i64 %592
  %594 = load i32, ptr %593, align 4
  %595 = getelementptr inbounds [5 x i32], ptr %9, i64 0, i64 1
  %596 = load i32, ptr %595, align 4
  %597 = icmp ne i32 %594, %596
  br i1 %597, label %598, label %599

598:                                              ; preds = %588
  store i32 0, ptr %4, align 4
  br label %708

599:                                              ; preds = %588
  %600 = load ptr, ptr %6, align 8
  %601 = load i32, ptr @pos, align 4
  %602 = add nsw i32 %601, 2
  %603 = sext i32 %602 to i64
  %604 = getelementptr inbounds i32, ptr %600, i64 %603
  %605 = load i32, ptr %604, align 4
  %606 = getelementptr inbounds [5 x i32], ptr %9, i64 0, i64 2
  %607 = load i32, ptr %606, align 8
  %608 = icmp ne i32 %605, %607
  br i1 %608, label %609, label %610

609:                                              ; preds = %599
  store i32 0, ptr %4, align 4
  br label %708

610:                                              ; preds = %599
  %611 = load ptr, ptr %6, align 8
  %612 = load i32, ptr @pos, align 4
  %613 = add nsw i32 %612, 3
  %614 = sext i32 %613 to i64
  %615 = getelementptr inbounds i32, ptr %611, i64 %614
  %616 = load i32, ptr %615, align 4
  %617 = getelementptr inbounds [5 x i32], ptr %9, i64 0, i64 3
  %618 = load i32, ptr %617, align 4
  %619 = icmp ne i32 %616, %618
  br i1 %619, label %620, label %621

620:                                              ; preds = %610
  store i32 0, ptr %4, align 4
  br label %708

621:                                              ; preds = %610
  %622 = load ptr, ptr %6, align 8
  %623 = load i32, ptr @pos, align 4
  %624 = add nsw i32 %623, 4
  %625 = sext i32 %624 to i64
  %626 = getelementptr inbounds i32, ptr %622, i64 %625
  %627 = load i32, ptr %626, align 4
  %628 = getelementptr inbounds [5 x i32], ptr %9, i64 0, i64 4
  %629 = load i32, ptr %628, align 16
  %630 = icmp ne i32 %627, %629
  br i1 %630, label %631, label %632

631:                                              ; preds = %621
  store i32 0, ptr %4, align 4
  br label %708

632:                                              ; preds = %621
  br label %633

633:                                              ; preds = %632
  br label %634

634:                                              ; preds = %633
  br label %635

635:                                              ; preds = %634
  br label %636

636:                                              ; preds = %635
  br label %637

637:                                              ; preds = %636
  %638 = load i32, ptr @pos, align 4
  %639 = add nsw i32 %638, 5
  store i32 %639, ptr @pos, align 4
  br label %701

640:                                              ; preds = %569
  %641 = load i32, ptr %5, align 4
  %642 = icmp eq i32 %641, 7
  br i1 %642, label %643, label %699

643:                                              ; preds = %640
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %10, ptr align 16 @__const.detect_item.mNull, i64 16, i1 false)
  %644 = load i32, ptr @pos, align 4
  %645 = add nsw i32 %644, 3
  %646 = load i32, ptr %7, align 4
  %647 = icmp sge i32 %645, %646
  br i1 %647, label %648, label %649

648:                                              ; preds = %643
  store i32 0, ptr %4, align 4
  br label %708

649:                                              ; preds = %643
  %650 = load ptr, ptr %6, align 8
  %651 = load i32, ptr @pos, align 4
  %652 = sext i32 %651 to i64
  %653 = getelementptr inbounds i32, ptr %650, i64 %652
  %654 = load i32, ptr %653, align 4
  %655 = getelementptr inbounds [4 x i32], ptr %10, i64 0, i64 0
  %656 = load i32, ptr %655, align 16
  %657 = icmp ne i32 %654, %656
  br i1 %657, label %658, label %659

658:                                              ; preds = %649
  store i32 0, ptr %4, align 4
  br label %708

659:                                              ; preds = %649
  %660 = load ptr, ptr %6, align 8
  %661 = load i32, ptr @pos, align 4
  %662 = add nsw i32 %661, 1
  %663 = sext i32 %662 to i64
  %664 = getelementptr inbounds i32, ptr %660, i64 %663
  %665 = load i32, ptr %664, align 4
  %666 = getelementptr inbounds [4 x i32], ptr %10, i64 0, i64 1
  %667 = load i32, ptr %666, align 4
  %668 = icmp ne i32 %665, %667
  br i1 %668, label %669, label %670

669:                                              ; preds = %659
  store i32 0, ptr %4, align 4
  br label %708

670:                                              ; preds = %659
  %671 = load ptr, ptr %6, align 8
  %672 = load i32, ptr @pos, align 4
  %673 = add nsw i32 %672, 2
  %674 = sext i32 %673 to i64
  %675 = getelementptr inbounds i32, ptr %671, i64 %674
  %676 = load i32, ptr %675, align 4
  %677 = getelementptr inbounds [4 x i32], ptr %10, i64 0, i64 2
  %678 = load i32, ptr %677, align 8
  %679 = icmp ne i32 %676, %678
  br i1 %679, label %680, label %681

680:                                              ; preds = %670
  store i32 0, ptr %4, align 4
  br label %708

681:                                              ; preds = %670
  %682 = load ptr, ptr %6, align 8
  %683 = load i32, ptr @pos, align 4
  %684 = add nsw i32 %683, 3
  %685 = sext i32 %684 to i64
  %686 = getelementptr inbounds i32, ptr %682, i64 %685
  %687 = load i32, ptr %686, align 4
  %688 = getelementptr inbounds [4 x i32], ptr %10, i64 0, i64 3
  %689 = load i32, ptr %688, align 4
  %690 = icmp ne i32 %687, %689
  br i1 %690, label %691, label %692

691:                                              ; preds = %681
  store i32 0, ptr %4, align 4
  br label %708

692:                                              ; preds = %681
  br label %693

693:                                              ; preds = %692
  br label %694

694:                                              ; preds = %693
  br label %695

695:                                              ; preds = %694
  br label %696

696:                                              ; preds = %695
  %697 = load i32, ptr @pos, align 4
  %698 = add nsw i32 %697, 4
  store i32 %698, ptr @pos, align 4
  br label %700

699:                                              ; preds = %640
  store i32 0, ptr %4, align 4
  br label %708

700:                                              ; preds = %696
  br label %701

701:                                              ; preds = %700, %637
  br label %702

702:                                              ; preds = %701, %566
  br label %703

703:                                              ; preds = %702, %507
  br label %704

704:                                              ; preds = %703, %387
  br label %705

705:                                              ; preds = %704, %318
  br label %706

706:                                              ; preds = %705, %271
  br label %707

707:                                              ; preds = %706
  store i32 1, ptr %4, align 4
  br label %708

708:                                              ; preds = %707, %699, %691, %680, %669, %658, %648, %631, %620, %609, %598, %587, %577, %561, %550, %539, %528, %518, %505, %497, %487, %477, %469, %462, %442, %432, %424, %417, %408, %386, %378, %368, %348, %339, %316, %308, %158, %149, %120, %116, %105, %94, %83, %72, %61, %49, %38, %27, %14
  %709 = load i32, ptr %4, align 4
  ret i32 %709
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %5 = call i32 @getch()
  store i32 %5, ptr %2, align 4
  store i32 0, ptr %3, align 4
  br label %6

6:                                                ; preds = %9, %0
  %7 = load i32, ptr %2, align 4
  %8 = icmp ne i32 %7, 35
  br i1 %8, label %9, label %17

9:                                                ; preds = %6
  %10 = load i32, ptr %2, align 4
  %11 = load i32, ptr %3, align 4
  %12 = sext i32 %11 to i64
  %13 = getelementptr inbounds [50000000 x i32], ptr @buffer, i64 0, i64 %12
  store i32 %10, ptr %13, align 4
  %14 = load i32, ptr %3, align 4
  %15 = add nsw i32 %14, 1
  store i32 %15, ptr %3, align 4
  %16 = call i32 @getch()
  store i32 %16, ptr %2, align 4
  br label %6, !llvm.loop !13

17:                                               ; preds = %6
  %18 = load i32, ptr %3, align 4
  call void @skip_space(ptr noundef @buffer, i32 noundef %18)
  %19 = load i32, ptr %3, align 4
  %20 = call i32 @detect_item(i32 noundef 0, ptr noundef @buffer, i32 noundef %19)
  store i32 %20, ptr %4, align 4
  %21 = load i32, ptr %3, align 4
  call void @skip_space(ptr noundef @buffer, i32 noundef %21)
  %22 = load i32, ptr %4, align 4
  %23 = icmp ne i32 %22, 0
  br i1 %23, label %24, label %25

24:                                               ; preds = %17
  call void @putch(i32 noundef 111)
  call void @putch(i32 noundef 107)
  call void @putch(i32 noundef 10)
  store i32 0, ptr %1, align 4
  br label %26

25:                                               ; preds = %17
  call void @putch(i32 noundef 110)
  call void @putch(i32 noundef 111)
  call void @putch(i32 noundef 116)
  call void @putch(i32 noundef 32)
  call void @putch(i32 noundef 111)
  call void @putch(i32 noundef 107)
  call void @putch(i32 noundef 10)
  store i32 1, ptr %1, align 4
  br label %26

26:                                               ; preds = %25, %24
  %27 = load i32, ptr %1, align 4
  ret i32 %27
}

declare i32 @getch() #2

declare void @putch(i32 noundef) #2

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
