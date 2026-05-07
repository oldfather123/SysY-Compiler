package tools;

import ir.Value;

import java.util.LinkedHashMap;
import java.util.Map;

// use name to set ID
// 由于存在同名变量出现在不同块中的情况 所以每一个basicBlock中都有一个IRregDispatcher
public class IrRegDispatcher {
    private final Map<String, Integer> allocator = new LinkedHashMap<>();

    public String allocId(String area) {
        if(allocator.containsKey(area)){
            allocator.merge(area, 1, Integer::sum);
        }else{
            allocator.put(area, 0);
        }
        return allocator.get(area).toString();
    }

    public String allocId() {
        return allocId("Default");
    }

    public IrRegDispatcher(){
    }

    public IrRegDispatcher(int ori){
        allocator.put("Default", ori);
    }
}
