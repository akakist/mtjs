registerMethod("recurse", function(p) {
    return callMethod("recursive", "recurse", {});
});