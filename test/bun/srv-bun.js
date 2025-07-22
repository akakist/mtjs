// Простой HTTP-сервер
Bun.serve({
  port: 3000,
  fetch(req) {
    const url = new URL(req.url);
    
    // Роутинг
    if (url.pathname === "/") {
      return new Response("Добро пожаловать на главную страницу! 🚀");
    }

    if (url.pathname === "/api") {
      return new Response(JSON.stringify({ message: "Привет от API!" }), {
        headers: { "Content-Type": "application/json" },
      });
    }

    return new Response("Страница не найдена", { status: 404 });
  },
  error(error) {
    return new Response("Ошибка сервера: " + error.toString(), { status: 500 });
  },
});

console.log("Сервер запущен на http://localhost:3000");