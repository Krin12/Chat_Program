#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 100
#define FILE_BUF_SIZE 1024

GtkWidget *text_view;
GtkWidget *entry;
GtkWidget *ip_entry;
GtkWidget *port_entry;
GtkWidget *name_entry;
GtkWidget *file_chooser;
int client_socket;

void *recv_message(void *arg);
void connect_to_server(GtkWidget *widget, gpointer data);
void show_connection_dialog(GtkWidget *widget, gpointer data);
void send_message(GtkWidget *widget, gpointer data);
void send_file(GtkWidget *widget, gpointer data);

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *connect_button;
    GtkWidget *file_button;
    pthread_t recv_thread;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Send");
    g_signal_connect(button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    file_button = gtk_button_new_with_label("Send File");
    g_signal_connect(file_button, "clicked", G_CALLBACK(send_file), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), file_button, FALSE, FALSE, 0);
    
    connect_button = gtk_button_new_with_label("Connect");
    g_signal_connect(connect_button, "clicked", G_CALLBACK(show_connection_dialog), window);
    gtk_box_pack_start(GTK_BOX(hbox), connect_button, FALSE, FALSE, 0);
    
    file_chooser = gtk_file_chooser_dialog_new("Choose File",
                                              GTK_WINDOW(window),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              "Cancel",
                                              GTK_RESPONSE_CANCEL,
                                              "Open",
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(file_chooser), FALSE);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}

void *recv_message(void *arg) {
    char recv_buf[BUF_SIZE];
    while (1) {
        int str_len = read(client_socket, recv_buf, sizeof(recv_buf));
        if (str_len <= 0)
            break;

        recv_buf[str_len] = '\0';

        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buffer, &iter);

        if (!gtk_text_iter_starts_line(&iter))
            gtk_text_buffer_insert(buffer, &iter, "\n", 1);

        gtk_text_buffer_insert(buffer, &iter, recv_buf, -1);
    }
    close(client_socket);
    exit(0);
}

void connect_to_server(GtkWidget *widget, gpointer data) {
    const gchar *server_ip = gtk_entry_get_text(GTK_ENTRY(ip_entry));
    const gchar *server_port = gtk_entry_get_text(GTK_ENTRY(port_entry));
    const gchar *client_name = gtk_entry_get_text(GTK_ENTRY(name_entry));

    if (client_socket != 0) {
        g_print("이미 서버에 연결되어 있습니다.\n");
        return;
    }

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(atoi(server_port));

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() error");
        exit(1);
    }

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_message, NULL);

    char name_msg[BUF_SIZE];
    snprintf(name_msg, sizeof(name_msg), "%s", client_name);
    write(client_socket, name_msg, strlen(name_msg));
    
    gtk_widget_destroy(GTK_WIDGET(data));
}

void show_connection_dialog(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label_ip, *label_port, *label_name;

    dialog = gtk_dialog_new_with_buttons("서버에 연결",
                                         GTK_WINDOW(data),
                                         GTK_DIALOG_MODAL,
                                         "Connect",
                                         GTK_RESPONSE_ACCEPT,
                                         "Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    label_ip = gtk_label_new("서버 IP:");
    ip_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), label_ip, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), ip_entry, FALSE, FALSE, 0);

    label_port = gtk_label_new("서버 포트:");
    port_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), label_port, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), port_entry, FALSE, FALSE, 0);

    label_name = gtk_label_new("사용자 이름:");
    name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(content_area), label_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), name_entry, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        connect_to_server(widget, dialog);
    } else {
        gtk_widget_destroy(dialog);
    }
}

void send_message(GtkWidget *widget, gpointer data) {
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(entry));
    write(client_socket, message, strlen(message));
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

void send_file(GtkWidget *widget, gpointer data) {
    gint result = gtk_dialog_run(GTK_DIALOG(file_chooser));

    if (result == GTK_RESPONSE_ACCEPT) {
        const gchar *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

        if (file_path == NULL) {
            g_print("파일을 선택하세요.\n");
            gtk_widget_hide(file_chooser);
            return;
        }

        FILE *file = fopen(file_path, "rb");
        if (file == NULL) {
            g_print("파일을 열 수 없습니다.\n");
            gtk_widget_hide(file_chooser);
            return;
        }

        // 파일명만 추출
        const gchar *file_name = g_path_get_basename(file_path);

        // 서버에게 파일 전송 시작을 알리는 메시지 전송
        write(client_socket, "/upload", strlen("/upload"));

        // 파일 정보 전송 (파일명, 파일크기)
        write(client_socket, file_name, BUF_SIZE);
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        write(client_socket, &file_size, sizeof(file_size));

        // 파일 전송
        char file_buffer[FILE_BUF_SIZE];
        size_t bytesRead;
        while ((bytesRead = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
            write(client_socket, file_buffer, bytesRead);
        }

        fclose(file);

        g_print("파일 전송이 완료되었습니다.\n");

        gtk_widget_hide(file_chooser);
    }
}
