package mid.IntermediatePresentation.Function;

import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

public class Param extends User {
    ArrayList<Value> params = new ArrayList<>();

    public Param() {
        super("PARAM", ValueType.NULL);
    }

    public Param(Value... params) {
        super("PARAM", ValueType.NULL);
        Collections.addAll(this.params, params);
        Arrays.stream(params).forEach(this::use);
    }

    public void addParam(Value param) {
        params.add(param);
    }

    public ArrayList<Value> getParams() {
        return params;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (Value param : params) {
            sb.append(param.getTypeString()).append(" ").append(param.getReg()).append(", ");
        }
        if (params.size() > 0) {
            return sb.substring(0, sb.length() - 2);
        } else {
            return "";
        }
    }

    public void destroy() {
        for (Value param : params) {
            param.removeUser(this);
        }
        params.clear();
    }
}
