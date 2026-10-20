registerMethod("badFloatLiteral",  function(p) { return 1.5; });
registerMethod("badFloatExponent", function(p) { return 1e300; });
registerMethod("badNaN",           function(p) { return NaN; });
registerMethod("badInfinity",      function(p) { return Infinity; });
registerMethod("badParseFloat",    function(p) { return parseFloat("1.5"); });
