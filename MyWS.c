#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/stat.h>

struct files {
    char* name;
    char* size;
    char* date;
};

void client(int client_sock, char* root);
void url_decode(char* str);
bool type_folder(char* root, char* path);
void get_props(char* root);
void sender(int client_sock,bool folder, char* root, char* path);
char* build();

struct files* act;
bool design;
int length;

int main(int argc, char* argv[])
{
    // Comprobar argumentos
    if (argc < 3) { perror("Non arguments found\n"); exit(1); }

    int port = atoi(argv[1]);
    char* root = argv[2];

    if (port == 0) { perror("Invalid Port\n"); exit(1); }
    DIR* dir = opendir(root);
    if (dir == NULL) { perror("Invalid Root\n"); exit(1); }
    closedir(dir);

    // Crear estructuras
    int server_sock, client_sock;
    struct sockaddr_in server_adr, client_adr;

    // Crear socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) { perror("Create socket error\n"); exit(1); }

    // Configurar servidor
    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(port);
    server_adr.sin_addr.s_addr = INADDR_ANY;

    // Vincular socket y servidor
    if (bind(server_sock, (struct sockaddr*) &server_adr, sizeof(server_adr)) < 0) { perror("Binding server error\n"); exit(1); }

    // Escuchar conexiones
    if (listen(server_sock, 5) < 0) { perror("Listening socket error\n"); exit(1); }

    socklen_t client_len = sizeof(client_adr);

    while(1)
    {
        // Aceptar conexiones
        client_sock = accept(server_sock, (struct sockaddr *)&client_adr, &client_len);
        if (client_sock < 0) { perror("Accept error\n"); exit(1); }

        pid_t pid = fork();
        if (pid == -1) { perror("Fork error\n"); exit(1); }

        // Manejar cliente
        if (pid == 0)
        {
            close(server_sock);
            client(client_sock, root);
            exit(0);
        }
        
        // Proceso padre sigue escuchando
        close(client_sock);
    }
    
    close(server_sock);
    return 0;
}

void client(int client_sock, char* root)
{
    // Crear buffer para peticiones
    char* buffer = (char*)malloc(1024 * sizeof(char));

    // Leer peticiones
    ssize_t bytes_recv;
    while((bytes_recv = recv(client_sock, buffer, 1024, 0)) > 0)
    {        
        // Crear estructuras
        design = false;
        bool folder;
        char newroot[256] = "";
        strcat(newroot, root);

        // Obtener el método HTTP
        char method[10];
        sscanf(buffer, "%s", method);

        // Obtener la ruta de la solicitud
        char path[1024];
        sscanf(buffer, "%*s %s", path);

        // Decodificar la ruta de la solicitud
        url_decode(path);

        if (strcmp(method, "GET") == 0)
        {
            // Determinar tipo
            folder = type_folder(newroot, path);

            if (folder)
            {
                DIR* dir = opendir(newroot);
                struct dirent* obj;
                length = 0;

                while((obj = readdir(dir)) != NULL)
                {
                    if ((strcmp(obj->d_name, ".") != 0) && (strcmp(obj->d_name, "..") != 0)) { length++; } 
                }
                closedir(dir);

                get_props(newroot);
            }  
        }
        sender(client_sock, folder, newroot, path);
    }

    close(client_sock);
}

void url_decode(char* str)
{
    char* p = str;
    char hex[3] = {0};
    while (*str)
    {
        if (*str == '%' && isxdigit((unsigned char)*(str + 1)) && isxdigit((unsigned char)*(str + 2)))
        {
            hex[0] = *(str + 1);
            hex[1] = *(str + 2);
            *p = strtol(hex, NULL, 16);
            str += 2;
        }
        else if (*str == '+') *p = ' ';
        else *p = *str;

        str++;
        p++;
    }
    *p = '\0';
}

bool type_folder(char* root, char* path)
{
    if (strcmp(path, "/favicon.ico") == 0)
    {
        design = true;
        return false;
    }
    else
    {
        strcat(root, path);
        DIR* dir = opendir(root);
        if (dir) { closedir(dir); return true; }
        else { return false; }
    }
}

void get_props(char* root)
{
    char size[20];
    char date[20];
    DIR* dir = opendir(root);
    struct dirent* obj;
    struct stat filestat;

    act = (struct files*)malloc(length * sizeof(struct files));
    int i = 0;

    while((obj = readdir(dir)) != NULL)
    {
        if ((strcmp(obj->d_name, ".") != 0) && (strcmp(obj->d_name, "..") != 0))
        {
            char temp[128];
            snprintf(temp, sizeof(temp), "%s/%s", root, obj->d_name);
            if (stat(temp, &filestat) == -1) 
            {
                perror("Unable to get file status");
                exit(1);
            }

            sprintf(size, "%ld", filestat.st_size);
            strftime(date, 20, "%y-%m-%d", localtime(&filestat.st_mtime));

            char* temp_size = strdup(size);
            char* temp_date = strdup(date);

            act[i].name = strdup(obj->d_name);
            act[i].size = temp_size;
            act[i].date = temp_date;
            i++;
        }        
    }
    closedir(dir);
}

void sender(int client_sock, bool folder, char* root, char* path)
{
    char* html;
    char answer[8192];

    if (folder)
    {
        html = build();
        sprintf(answer, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "Content-Length: %ld\r\n\r\n"
                        "%s", strlen(html), html);

        send(client_sock, answer, strlen(answer), 0);
    }
    else
    {
        FILE* file = fopen(root, "rb");
        if (file == NULL || design)
        {
            char error_answer[128];
            sprintf(error_answer,   "HTTP/1.1 404 Not Found\r\n"
                                    "Content-Length: %d\r\n\r\n"
                                    "File not found or cannot be opened", 32);
            send(client_sock, error_answer, strlen(error_answer), 0);
        }
        else 
        {
            // Obtener el tamaño del archivo
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            // Leer el contenido del archivo en un búfer
            char* file_buffer = (char*)malloc(file_size);
            fread(file_buffer, file_size, 1, file);
                    
            // Construir la respuesta HTTP con el contenido del archivo
            sprintf(answer, "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/octet-stream; charset=UTF-8\r\n"
                            "Content-Disposition: attachment; filename=\"%s\"\r\n"
                            "Content-Length: %ld\r\n\r\n", path, file_size);
            send(client_sock, answer, strlen(answer), 0);

            // Enviar el contenido del archivo al cliente
            send(client_sock, file_buffer, file_size, 0);
            
            // Liberar memoria y cerrar el archivo
            free(file_buffer);
            fclose(file);
        } 
    }
}

char* build()
{
    char* html = (char*)malloc(8192 * sizeof(char));
    strcat(html, "<!DOCTYPE html>"
            "<html>"
            "<head>"
                "<style>"
                    "body {"
                        "background-color:SlateBlue;"
                    "}"
                    "h1 {"
                        "color: #000;"
                        "font-size: 40px;"
                        "text-align: center;"
                    "}"
                    "table {"
                        "color: #000;"
                        "width: 80%;"
                        "overflow: hidden;"
                    "}"
                    "th, td {"
                        "padding: 4px;"
                        "text-align: left;"
                    "}"
                    "th {"
                        "font-size: 25px;"
                    "}"
                    "tr:hover {"
                        "background-color: #ababab;"
                        "cursor: pointer;"
                    "}"
                "</style>"
                "<title>My WebServer</title>"
            "</head>"
            "<body>"
                "<h1>FTP La Habana</h1>"
                "<table id=\"myTable\">"
                    "<tr>"
                        "<th onclick=\"sortTable(0, 'str')\">Nombre</th>"
                        "<th onclick=\"sortTable(1, 'int')\">Size</th>"
                        "<th onclick=\"sortTable(2, 'str')\">Date</th>"
                    "</tr>");
    for (int i = 0; i < length; i++) {
        strcat(html, "<tr onclick=\"sendRequest('");
        strcat(html, act[i].name);
        strcat(html, "')\"><td>");        
        strcat(html, act[i].name);
        strcat(html, "</td><td>");
        strcat(html, act[i].size);
        strcat(html, "</td><td>");
        strcat(html, act[i].date);
        strcat(html, "</td></tr>");
    }
    strcat(html, 
                "</table>"
                "<script>"
                    "function sendRequest(name) {"
                        "var current = window.location.href;"
                        "console.log(current);"
                        "if(current[current.length-1] == '/')"
                        "{ window.location.href = current + name; }"
                        "else window.location.href = current + '/' + name;"
                    "}"
                    "function sortTable(n,type) {"
                        "var table, rows, switching, i, x, y, shouldSwitch, dir, switchcount = 0;"
                        "table = document.getElementById(\"myTable\");"
                        "switching = true;"
                        "dir = \"asc\";"
                        "while (switching) {"
                            "switching = false;"
                            "rows = table.rows;"
                            "for (i = 1; i < (rows.length - 1); i++) {"
                                "shouldSwitch = false;"
                                "x = rows[i].getElementsByTagName(\"TD\")[n];"
                                "y = rows[i + 1].getElementsByTagName(\"TD\")[n];"
                                "if (dir == \"asc\") {"
                                    "if ((type==\"str\" && x.innerHTML.toLowerCase() > y.innerHTML.toLowerCase()) || (type==\"int\" && parseFloat(x.innerHTML) > parseFloat(y.innerHTML))) {"
                                        "shouldSwitch= true;"
                                        "break;"
                                    "}"
                                "} else if (dir == \"desc\") {"
                                    "if ((type==\"str\" && x.innerHTML.toLowerCase() < y.innerHTML.toLowerCase()) || (type==\"int\" && parseFloat(x.innerHTML) < parseFloat(y.innerHTML))) {"
                                    "shouldSwitch = true;"
                                    "break;"
                                    "}"
                                "}"
                            "}"
                            "if (shouldSwitch) {"
                                "rows[i].parentNode.insertBefore(rows[i + 1], rows[i]);"
                                "switching = true;"
                                "switchcount ++;"
                            "} else {"
                                "if (switchcount == 0 && dir == \"asc\") {"
                                    "dir = \"desc\";"
                                    "switching = true;"
                                "}"
                            "}"
                        "}"
                    "}"
                "</script>"
            "</body>"
            "</html>");

    return html;
}
