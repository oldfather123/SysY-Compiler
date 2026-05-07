package Utils;

import java.util.Objects;

/**
 * A simple generic container for two related objects.
 */
public record ImmutablePair<T1, T2>(T1 left, T2 right) {

    public ImmutablePair(T1 left, T2 right) {
        this.left = Objects.requireNonNull(left, "Left element cannot be null");
        this.right = Objects.requireNonNull(right, "Right element cannot be null");
    }

    @Override
    public String toString() {
        return "(" + left + ", " + right + ")";
    }
}