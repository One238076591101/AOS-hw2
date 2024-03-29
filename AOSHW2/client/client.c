// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define MAX_BUFFER_SIZE 1024

void sendCommand(int client_socket, const char *command) {
    // 發送資料到伺服器
    if (send(client_socket, command, strlen(command), 0) == -1) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    printf("Command sent to server: %s\n", command);

}

void receiveFile(int client_socket) {
    char filename[MAX_BUFFER_SIZE];
    
    // 從伺服器接收檔案名稱
    ssize_t received_bytes = recv(client_socket, filename, sizeof(filename), 0);
    // 添加 null 結尾
    filename[received_bytes] = '\0';

    if (received_bytes < 0) {
        perror("Error receiving file name");
        return;
    }

    // 根據接收到的檔案名稱建立或開啟文件
    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    char buffer[MAX_BUFFER_SIZE];
    ssize_t remaining_bytes;

    printf("成功讀取檔案 %s\n", filename);
    // 使用循環接收文件內容
    while ((remaining_bytes = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, remaining_bytes, file);
    }
   

    if (remaining_bytes == -1) {
        perror("Error receiving file");
    }

    fclose(file);
}

void print_usage()
{
	printf("%s\n","[ Program command example ]");
	printf("%s\n","Format :");
	printf("%s\n","1) create homework2.c rwr---");
	printf("%s\n","2) read homework2.c");
	printf("%s\n","3) write homework2.c o/a");
	printf("%s\n","4) changemode homework2.c rw----");
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Setup server address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // 提示使用者輸入數字（1~6）
    int userChoice;
        
    printf("**********client list**********\n");
    printf("1:AOS-anne 2:AOS-ben 3:AOS-claire\n");
    printf("4:CSE-dennis 5:CSE-eric 6:CSE-fred\n");
    printf("Enter a client number (1~6): ");
    scanf("%d", &userChoice);

    // Add getchar to absorb newline
    getchar();

    // 定義groups和members訊息
    const char *groups[] = {"AOS-members", "CSE-members"};
    const char *members[][3] = {{"anne", "ben", "claire"}, {"dennis", "eric", "fred"}};

    // Validate user choice
    if (userChoice < 1 || userChoice > 6) {
        fprintf(stderr, "Invalid number. Please enter a number between 1 and 6.\n");
        exit(EXIT_FAILURE);
    }

    // 根據使用者選擇群組和成員
    const char *group, *member;
    if (userChoice <= 3) {
        group = groups[0]; // AOS-members
        member = members[0][userChoice - 1]; // Adjust index
    } else {
        group = groups[1]; // CSE-members
        member = members[1][userChoice - 4]; // Adjust index
    }

    // Prompt user to input a command
    print_usage();
    printf("Enter a command: ");
    char command[MAX_BUFFER_SIZE];

    // 使用 fgets 讀取使用者輸入
    if (fgets(command, MAX_BUFFER_SIZE, stdin) == NULL) {
        fprintf(stderr, "Error reading user input.\n");
        exit(EXIT_FAILURE);
    }

    // Remove newline characters
    command[strcspn(command, "\n")] = '\0';

    // Create the command using the determined group and member information
    char createCommand[MAX_BUFFER_SIZE];
    int chars_written = snprintf(createCommand, MAX_BUFFER_SIZE, "%s,%s,%s", command, group, member);

    if (chars_written < 0 || chars_written >= MAX_BUFFER_SIZE) {
        fprintf(stderr, "Error creating command. Resulting string too long.\n");
        exit(EXIT_FAILURE);
    }

    // 發送命令到伺服器
    sendCommand(client_socket, createCommand);
    receiveFile(client_socket);
    // 進入無窮迴圈，保持主程式運行
    while (1) {
        
        // 等待或處理其他事件，例如接收其他指令，保持主程式不結束
    }

    // Close socket
    close(client_socket);

    return 0;
}