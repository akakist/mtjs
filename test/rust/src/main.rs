use actix_web::{get, App, HttpResponse, HttpServer, Responder};
use std::io;

#[get("/")]
async fn index() -> impl Responder {
    HttpResponse::Ok().body("Hello, World!")
}

#[actix_web::main]
async fn main() -> io::Result<()> {
    println!("Starting server at http://127.0.0.1:8080");
    HttpServer::new(|| App::new().service(index))
        .bind("127.0.0.1:8080")?
        .run()
        .await
}