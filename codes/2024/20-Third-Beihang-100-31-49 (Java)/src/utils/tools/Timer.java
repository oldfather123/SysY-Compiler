package utils.tools;

import mid.Optimizer.HyperParams;

public class Timer {
    private long startTime;
    public static Timer INSTANCE = new Timer();

    private Timer() {
    }

    public void start() {
        startTime = System.currentTimeMillis();
    }

    public boolean timeOut() {
        return (System.currentTimeMillis() - startTime) / 1000 > HyperParams.TIMEOUT;
    }
}
