# AprilTag Recommendations for Different Use Cases

## 🎯 For 50m Detection (Your Goal)

| Family | Min Tag Size for 50m | Physical Size | Unique IDs | Recommendation |
|--------|---------------------|---------------|------------|----------------|
| **Tag16h5** | **1.1m** black square | **1.4m total** | 30 tags | 🥇 **BEST** - Smallest physical tag |
| Tag25h9 | 1.7m black square | 2.1m total | 35 tags | 🥈 Good compromise |
| Tag36h11 | 2.4m black square | 3.0m total | 587 tags | 🥉 Requires largest tag |

**🔥 Key Insight**: Tag16h5 needs **54% smaller** physical tag than Tag36h11 for same 50m detection!

## 🎯 Recommended Configurations

### Long Distance Detection (50m+)
```cpp
CURRENT_TAG_SIZE = TAG_1000MM_BLACK;  // 1m black square (1.25m total)
CURRENT_TAG_FAMILY = FAMILY_16H5;     // Large features
// Result: 50m detection with manageable 1.25m total tag size
```

### Drone Landing Approach
```cpp
// Phase 1: Initial detection (20-50m)
CURRENT_TAG_SIZE = TAG_300MM_BLACK;   // 30cm black square
CURRENT_TAG_FAMILY = FAMILY_16H5;     // Maximum range
// Detection range: 4.2m - 21m (with 2.25x boost)

// Phase 2: Precision landing (0.5-5m)
CURRENT_TAG_SIZE = TAG_100MM_BLACK;   // 10cm black square
CURRENT_TAG_FAMILY = FAMILY_36H11;    // High precision
// Detection range: 0.6m - 3.1m
```

### Indoor Robotics
```cpp
CURRENT_TAG_SIZE = TAG_100MM_BLACK;   // 10cm black square
CURRENT_TAG_FAMILY = FAMILY_36H11;    // Many unique IDs
// Detection range: 0.6m - 3.1m, 587 unique tags available
```

## 📏 Tag Size vs Family Comparison

For **50m minimum detection**, you need these **minimum black square sizes**:

| Family | Black Square | Total Size | Feature Size | IDs Available |
|--------|-------------|------------|--------------|---------------|
| Tag16h5 | 1.1m | 1.4m | **Large** (16×16) | 30 |
| Tag25h9 | 1.7m | 2.1m | Medium (25×25) | 35 |
| Tag36h11 | 2.4m | 3.0m | Small (36×36) | 587 |

## 🎯 Current Setup Analysis

**Your current configuration:**
- Tag: Tag36h11, 95mm black square (119mm total)
- Detection range: ~0.6m - 3.1m

**To achieve 50m detection, you could:**

1. **Keep Tag36h11** → Use 2.4m black square tag (3m total)
2. **Switch to Tag25h9** → Use 1.7m black square tag (2.1m total)
3. **Switch to Tag16h5** → Use 1.1m black square tag (1.4m total) ⭐ **Recommended**

## 🔧 How to Change Configuration

Edit `/ESPCAMTRIP/config.h`:

```cpp
// For 50m detection with smallest physical tag:
CURRENT_TAG_SIZE = TAG_1000MM_BLACK;  // 1m black square
CURRENT_TAG_FAMILY = FAMILY_16H5;     // Large features

// For your current 95mm tag:
CURRENT_TAG_SIZE = TAG_95MM_BLACK;    // 95mm black square
CURRENT_TAG_FAMILY = FAMILY_36H11;    // Small features (current)
```

The system will automatically:
- Calculate correct distances
- Show detection ranges
- Display configuration at startup

## 🎯 Bottom Line

**For 50m detection**: Use Tag16h5 family with 1m+ black square tags for optimal results!