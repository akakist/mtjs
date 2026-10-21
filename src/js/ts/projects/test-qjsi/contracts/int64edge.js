registerMethod("negOfInt64Min", function() {
    let min = -9223372036854775807 - 1;   // INT64_MIN
    return -min;                           // должен бросить RangeError
});
registerMethod("incOfInt64Max", function() {
    let max = 9223372036854775807;
    return max + 1;                        // должен бросить RangeError
});
registerMethod("decOfInt64Min", function() {
    let min = -9223372036854775807 - 1;
    return min - 1;                        // должен бросить RangeError
});
registerMethod("negOfInt64Max", function() {
    let max = 9223372036854775807;
    return -max;                           // валидно → -INT64_MAX
});