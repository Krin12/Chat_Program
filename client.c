#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 100
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3428

GtkWidget *text_view;
GtkWidget *entry;
int client_socket;

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
        
        // Add a new line before each message
        if (!gtk_text_iter_starts_line(&iter))
            gtk_text_buffer_insert(buffer, &iter, "\n", 1);

        gtk_text_buffer_insert(buffer, &iter, recv_buf, -1);
    }
    close(client_socket);
    exit(0);
}

static void send_message(GtkWidget *widget, gpointer data) {
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(entry));
    write(client_socket, message, strlen(message));
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button;
    pthread_t recv_thread;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
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

    gtk_widget_show_all(window);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() error");
        exit(1);
    }

    pthread_create(&recv_thread, NULL, recv_message, NULL);

    gtk_main();

    return 0;
}

