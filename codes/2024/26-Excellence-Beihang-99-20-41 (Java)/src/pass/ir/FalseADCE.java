package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Value;
import pass.Pass;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class FalseADCE implements Pass.IrPass{

    HashMap<String,Integer> ifCount = new HashMap<>();

    @Override
    public void run(IrModule module) {
        // do nothing
        for (Function function : module.getFunctions()) {
            passStepOne(function);
            passStepTwo(function);
        }
    }

    public void passStepOne(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (IrInstr instr : basicBlock.getInstrs()) {
                String instrStr = instr.toString();
                // 正则表达式识别 %数字或字母
                // 定义正则表达式模式
                String regex = "%[a-zA-Z0-9]\\\\d+";

                // 编译正则表达式
                Pattern pattern = Pattern.compile(regex);

                // 创建 Matcher 对象
                Matcher matcher = pattern.matcher(instrStr);
                while (matcher.find()) {
                    String group = matcher.group();
                    if (group.startsWith("%")) {
                        ifCount.put(group,ifCount.getOrDefault(group,0)+1);
                    }
                }
            }
        }
    }

    public void passStepTwo(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            ArrayList<IrInstr> instrsToMaintain = new ArrayList<>();
            for (IrInstr instr : basicBlock.getInstrs()) {
                String instrStr = instr.toString();
                // 正则表达式获取instrStr中的所有 %数字 结构
                String regex = "%[a-zA-Z0-9]\\\\d+";
                Pattern pattern = Pattern.compile(regex);
                Matcher matcher = pattern.matcher(instrStr);
                boolean flag = true;
                while (matcher.find()) {
                    if (matcher.group().startsWith("%")) {
                        if (ifCount.get(matcher.group()) == 1) {
                            flag = false;
                            break;
                        }
                        break;
                    }
                }
                if (flag) {
                    instrsToMaintain.add(instr);
                }
            }
            basicBlock.resetInstrs(instrsToMaintain);
        }
    }

    @Override
    public String getName() {
        return "false-ADCE";
    }
}
