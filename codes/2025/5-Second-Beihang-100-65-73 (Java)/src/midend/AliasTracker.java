package midend;

import frontend.ir.Value;
import frontend.ir.objecttype.derived.TPointer;

import java.util.*;

/**
 * 管理所有 Value 指针变量的属性标签，并提供别名判断接口。
 * 标签表示此指针可能指向的内存区域或语义组，用于支持 TBAA 和属性传播。
 */
public class AliasTracker {

    // 显式标记为 "互不别名" 的标签对
    private final Set<Long> disjointTagPairs = new HashSet<>();

    // 多标签不交集群组（组内任意两两不 alias）
    private final List<Set<Integer>> disjointGroups = new ArrayList<>();
    private final Map<Integer, List<Integer>> tagToGroupIds = new HashMap<>(); // tag -> [groupId]


    // 每个指针变量对应的标签集合（可能属于多个标签组）
    public final Map<Value, Set<Integer>> valueTagMap = new HashMap<>();

    // notAlias 结果缓存：<minId << 32 | maxId> -> true
    private final Set<Long> notAliasCache = new HashSet<>();

    public static final Set<Integer> EMPTY_TAGS = Collections.emptySet();

    /**
     * 编码两个指针值的唯一无序组合 key（用于缓存）
     */
    private static long encode(Value a, Value b) {
        return (long) System.identityHashCode(a) << 32 | System.identityHashCode(b);
    }

    /**
     * 编码两个 tag 的唯一无序组合 key（用于 disjointPairs）
     */
    private static long encode(int a, int b) {
        return (long) Math.min(a, b) << 32 | Math.max(a, b);
    }

    /**
     * 清空缓存的方法
     */
    private void invalidateAliasCache() {
        notAliasCache.clear();
    }

    /**
     * 为两个标签添加不相交信息：用于构建 disjoint 集合
     */
    public void markTagsDisjoint(int tagA, int tagB) {
        if (tagA == tagB) {
            throw new IllegalArgumentException("Cannot mark same tag as disjoint: " + tagA);
        }
        disjointTagPairs.add(encode(tagA, tagB));
    }

    /**
     * 为两组标签全添加不相交信息：用于构建 disjoint 集合
     */
    public void markTagsDisjoint(Set<Integer> tags1, Set<Integer> tags2) {
        for (int tagA : tags1) {
            for (int tagB : tags2) {
                markTagsDisjoint(tagA, tagB);
            }
        }
    }

    /**
     * 为一组标签声明它们是互不别名的一组（任意两两不 alias）
     */
    public void addDisjointGroup(Set<Integer> group) {
        int groupId = disjointGroups.size();
        disjointGroups.add(new HashSet<>(group));
        for (int tag : group) {
            tagToGroupIds.computeIfAbsent(tag, _ -> new ArrayList<>()).add(groupId);
        }
    }

    /**
     * 给指针变量绑定标签集合（初始设定）
     */
    public void bindTags(Value ptr, Set<Integer> tags) {
        if (!(ptr.getType() instanceof TPointer)) {
            throw new IllegalArgumentException("Only pointer values can be bound with alias tags");
        }
        valueTagMap.put(ptr, new HashSet<>(tags));
        invalidateAliasCache();
    }

    /**
     * 将某些标签追加给指针变量；若有新增，返回 true
     */
    public boolean appendTags(Value ptr, Collection<Integer> newTags) {
        if (newTags.isEmpty()) return false;
        Set<Integer> tags = valueTagMap.computeIfAbsent(ptr, k -> new TreeSet<>());
        int before = tags.size();
        tags.addAll(newTags);
        invalidateAliasCache();
        return tags.size() != before;
    }

    /**
     * 将单个标签追加给指针变量
     */
    public boolean appendTag(Value ptr, int tag) {
        invalidateAliasCache();
        return valueTagMap.computeIfAbsent(ptr, k -> new TreeSet<>()).add(tag);
    }

    /**
     * 查询一个变量当前拥有的所有标签（可能为空）
     */
    public Set<Integer> getTags(Value ptr) {
        return valueTagMap.getOrDefault(ptr, EMPTY_TAGS);
    }

    /**
     * 判定两个指针变量是否一定不 alias（有 disjoint tag 即成立）
     */
    public boolean notAlias(Value a, Value b) {
        if (a == b) return false;
        long key = encode(a, b);
        if (notAliasCache.contains(key)) return true;

        Set<Integer> tagsA = valueTagMap.get(a);
        Set<Integer> tagsB = valueTagMap.get(b);
        if (tagsA == null || tagsB == null) return false;

        for (int ta : tagsA) {
            for (int tb : tagsB) {
                if (ta == tb) continue;
                if (disjointTagPairs.contains(encode(ta, tb)) || tagsInSameDisjointGroup(ta, tb)) {
                    notAliasCache.add(key);
                    return true;
                }
            }
        }

        return false;
    }


    /**
     * 判定两个指针是否一定 alias
     */
    public boolean mustAlias(Value a, Value b) {
        if (a == b) return true;
        Set<Integer> tagsA = getTags(a);
        Set<Integer> tagsB = getTags(b);
        return tagsA.equals(tagsB) && tagsA.size() == 1;
    }

    /**
     * 检查两个 tag 是否在某个 disjointGroup 中共现（不同标签）
     */
    private boolean tagsInSameDisjointGroup(int tagA, int tagB) {
        List<Integer> groupsA = tagToGroupIds.getOrDefault(tagA, List.of());
        List<Integer> groupsB = tagToGroupIds.getOrDefault(tagB, List.of());
        for (int g : groupsA) {
            if (groupsB.contains(g)) return true;
        }
        return false;
    }

}
