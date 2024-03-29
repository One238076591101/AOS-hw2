#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/file.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 5
#define PORT 8080

#define MAX_FILES 10
#define MAX_FILENAME_LEN 256
#define MAX_PERMISSION_LEN 7
#define MAX_DATA 100

// 這裡新增一個結構來儲存 client_socket 和對應的 port
struct ClientInfo {
    int client_socket;
    int port;
};
// 檔案結構
struct File {
    char filename[MAX_FILENAME_LEN];
    char owner[MAX_FILENAME_LEN];
    char group[MAX_FILENAME_LEN];
    char permissions[MAX_PERMISSION_LEN];
    int size;
    char *content;  
    bool isBeingRead;
    bool isBeingWritten;
};
// CapabilityList結構
struct CapabilityList {
    char filename[MAX_FILENAME_LEN];
    char owner[MAX_FILENAME_LEN];
    char group[MAX_FILENAME_LEN];
    char permissions[MAX_PERMISSION_LEN];
};

struct File files[MAX_FILES];
struct CapabilityList capabilityList[MAX_FILES];
int totalFiles = 0;


void createFile(char* filename, char* owner, char* group, char* permissions) {
    // 檢查是否超過檔案數量上限
    if (totalFiles >= MAX_FILES) {
        printf("無法創建新檔案，已達到檔案數量上限\n");
        return;
    }

    // 檢查檔案是否已存在
    for (int i = 0; i < totalFiles; i++) {
        if (strcmp(files[i].filename, filename) == 0) {
            printf("檔案 %s 已存在\n", filename);
            return;
        }
    }

    // 將檔案資訊添加到 files 陣列
    strcpy(files[totalFiles].filename, filename);
    strcpy(files[totalFiles].owner, owner);
    strcpy(files[totalFiles].group, group);
    strcpy(files[totalFiles].permissions, permissions);
    files[totalFiles].size = 0;  // 初始化檔案大小

    // 打開檔案
    FILE *file = fopen(filename, "w");
    
    // 檢查檔案是否成功開啟
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    // 寫入內容到檔案
    fprintf(file, "This is a sample content.\n");
    
    // 關閉檔案
    fclose(file);

    // 重新打開檔案，這次用 "r" 模式讀取
    file = fopen(filename, "r");

    // 檢查檔案是否成功開啟
    if (file == NULL) {
        perror("Error opening file for reading");
        return;
    }

    // 將內容存到 files[totalFiles].content
    fseek(file, 0, SEEK_END);  // 移到檔案末尾
    long file_size = ftell(file);
    rewind(file);  // 移回檔案開頭

    // 分配足夠的記憶體空間給 content
    files[totalFiles].content = malloc(file_size + 1);

    // 檢查是否成功分配記憶體
    if (files[totalFiles].content == NULL) {
        perror("Error allocating memory");
        return;
    }

    // 讀取檔案內容到 content
    fread(files[totalFiles].content, 1, file_size, file);

    // 在 content 的最後加上 null 結尾，確保它是一個有效的字串
    files[totalFiles].content[file_size] = '\0';

    // 關閉檔案
    fclose(file);

    char chmod_permissions[10];
    strcpy(chmod_permissions, files[totalFiles].permissions);

    // 使用 chmod 設置檔案權限
    int mode = 0;

    int count_r = 0;
    int count_w = 0;

    for (int i = 0; i < strlen(chmod_permissions); i++) {
        if (chmod_permissions[i] == 'r') {
            if (count_r == 0) {
                mode |= S_IRUSR;
            } else if (count_r == 1) {
                mode |= S_IRGRP;
            } else if (count_r == 2) {
                mode |= S_IROTH;
            }
            count_r++;
        } else if (chmod_permissions[i] == 'w') {
            if (count_w == 0) {
                mode |= S_IWUSR;
            } else if (count_w == 1) {
                mode |= S_IWGRP;
            } else if (count_w == 2) {
                mode |= S_IWOTH;
            }
            count_w++;
        }
    }
    chmod(filename, mode);

    // 將檔案權限添加到 capabilityList 陣列
    strcpy(capabilityList[totalFiles].filename, filename);
    strcpy(capabilityList[totalFiles].owner, owner);
    strcpy(capabilityList[totalFiles].group, group);
    strcpy(capabilityList[totalFiles].permissions, permissions);
    printf("***CAPABILITY LISTS*****\n");
    printf("filename: %s\n", capabilityList[totalFiles].filename);
    printf("owner: %s\n",  capabilityList[totalFiles].owner);
    printf("group: %s\n", capabilityList[totalFiles].group);
    printf("permissions: %s\n", capabilityList[totalFiles].permissions);

    printf("%s成功創建檔案 %s\n", owner, filename);

    totalFiles++;
}

void readFile(int client_socket, char *filename, char *username, char *group) {
    // 尋找檔案
    int fileIndex = -1;
    for (int i = 0; i < totalFiles; i++) {
        if (strcmp(files[i].filename, filename) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("找不到檔案 %s\n", filename);
        return;
    }

    // 檢查權限
    if (strcmp(group, files[fileIndex].group) != 0) { // 不同group
        printf("%s無權限讀取檔案 %s\n", username, filename);
        return;
    } else if (strcmp(group, files[fileIndex].group) == 0 && (strcmp(files[fileIndex].permissions, "r-----") == 0 || strcmp(files[fileIndex].permissions, "rw----") == 0)) { // 相同group看權限
        printf("%s無權限讀取檔案 %s\n", username, filename);
        return;
    }

    // 檢查檔案是否正在被寫入
    if (files[fileIndex].isBeingWritten) {
        printf("檔案 %s 正在被其他客戶端寫入，請稍後再試\n", filename);
        return;
    }
    // 標記檔案為正在被讀取
    files[fileIndex].isBeingRead = true;

    // 打開檔案
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        perror("Error opening file");

        // 還原檔案狀態
        files[fileIndex].isBeingRead = false;
        return;
    }

    // 在實際應用中，先傳送檔案名稱給客戶端
    write(client_socket, filename, strlen(filename));

    // 休眠一段時間，以防止檔案名稱和內容結合在一起
    sleep(1);

    // 使用 sendfile 將檔案內容傳送到客戶端
    off_t offset = 0;
    struct stat file_stat;
    fstat(file, &file_stat);

    ssize_t sent_bytes = sendfile(client_socket, file, &offset, file_stat.st_size);

    if (sent_bytes == -1) {
        perror("Error sending file");
    }

    sleep(3);

    // 關閉檔案
    close(file);

    // 還原檔案狀態
   files[fileIndex].isBeingRead = false;

    printf("%s成功讀取檔案 %s\n", username, filename);
}



void writeFile(char *filename, char *username, char *mode, char *group) {
    // 尋找檔案
    int fileIndex = -1;
    for (int i = 0; i < totalFiles; i++) {
        if (strcmp(files[i].filename, filename) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("找不到檔案 %s\n", filename);
        return;
    }

    // 檢查權限
    if (strcmp(group, files[fileIndex].group) != 0) { // 不同群組
        printf("%s無權限寫入檔案 %s\n", username, filename);
        return;
    } else if (strcmp(group, files[fileIndex].group) == 0 && (strcmp(files[fileIndex].permissions, "r-----") == 0 || strcmp(files[fileIndex].permissions, "r-r---") == 0 || strcmp(files[fileIndex].permissions, "rwr---") == 0)) { // 相同群組看權限
        printf("%s無權限寫入檔案 %s\n", username, filename);
        return;
    }

    // Check if the file is being written by another client
    if (files[fileIndex].isBeingWritten || files[fileIndex].isBeingRead) {
        printf("檔案 %s 正在被其他客戶端讀取或寫入，請稍後再試\n", filename);
        return;
    }

    // Set the flag to indicate that the file is being written
    files[fileIndex].isBeingWritten = true;
  
    // 打開檔案
    FILE *file;

    if (strcmp(mode, "o") == 0) {
        file = fopen(filename, "w");
    } else if (strcmp(mode, "a") == 0) {
        file = fopen(filename, "a");
    } else {
        printf("未知的寫入模式\n");

        // Reset the flag since we are not proceeding with writing
        files[fileIndex].isBeingWritten = false;

        return;
    }

    // 檢查檔案是否成功開啟
    if (file == NULL) {
        perror("Error opening file");

        // Reset the flag since we are not proceeding with writing
        files[fileIndex].isBeingWritten = false;

        return;
    }

    // 打開文字檔案
    FILE *txtFile = fopen("example.txt", "r");
    if (txtFile == NULL) {
        perror("Error opening txt file");

        // Reset the flag since we are not proceeding with writing
        files[fileIndex].isBeingWritten = false;

        // 關閉檔案
        fclose(file);
        return;
    }

    // Calculate the size of the file
    fseek(txtFile, 0, SEEK_END);
    long txtFileSize = ftell(txtFile);
    rewind(txtFile);

    // Allocate memory for content
    files[fileIndex].content = (char *)malloc(txtFileSize + 1);
    if (files[fileIndex].content == NULL) {
        perror("Memory allocation error");

        // Reset the flag since we are not proceeding with writing
        files[fileIndex].isBeingWritten = false;

        fclose(txtFile);
        fclose(file);
        return;
    }

    // Read the content of the txtFile into content
    size_t bytesRead = fread(files[fileIndex].content, 1, txtFileSize, txtFile);
    if (bytesRead != txtFileSize) {
        perror("Error reading from txt file");
        free(files[fileIndex].content);

        // Reset the flag since we are not proceeding with writing
        files[fileIndex].isBeingWritten = false;

        fclose(txtFile);
        fclose(file);
        return;
    }

    // Null-terminate the content
    files[fileIndex].content[txtFileSize] = '\0';

    // Write content to the file
    fprintf(file, "%s", files[fileIndex].content);

    // 關閉文字檔案
    fclose(txtFile);

    sleep(3);

    // 關閉檔案
    fclose(file);

    files[fileIndex].size = strlen(files[fileIndex].content); // 更新檔案大小
    // Reset the flag since writing is complete
    files[fileIndex].isBeingWritten = false;

    printf("%s成功寫入檔案 %s\n", username, filename);
}



void changeMode(char* filename, char* username, char* group, char* newPermissions) {
    // 尋找檔案
    int fileIndex = -1;
    for (int i = 0; i < totalFiles; i++) {
        if (strcmp(files[i].filename, filename) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("找不到檔案 %s\n", filename);
        return;
    }

    // 檢查權限(文件擁有者才能改變權限)
    if (strcmp(username, files[fileIndex].owner) != 0 ) {
        printf("%s無權限修改檔案 %s\n", username, filename);
        return;
    }

    char chmod_permissions[10];
    strcpy(chmod_permissions, newPermissions);

    // 使用 chmod 設置更新檔案權限
    mode_t mode = 0;

    int count_r = 0;
    int count_w = 0;

    for (int i = 0; i < strlen(chmod_permissions); i++) {
        if (chmod_permissions[i] == 'r') {
            if (count_r == 0) {
                mode |= S_IRUSR;
            } else if (count_r == 1) {
                mode |= S_IRGRP;
            } else if (count_r == 2) {
                mode |= S_IROTH;
            }
            count_r++;
        } else if (chmod_permissions[i] == 'w') {
            if (count_w == 0) {
                mode |= S_IWUSR;
            } else if (count_w == 1) {
                mode |= S_IWGRP;
            } else if (count_w == 2) {
                mode |= S_IWOTH;
            }
            count_w++;
        }
    }
    chmod(filename, mode);

    // 更新檔案權限
    strcpy(files[fileIndex].permissions, newPermissions);
    // 更新 capabilityList 陣列中的相應檔案的權限
    
    // 將檔案更新權限添加到 capabilityList 陣列
    strcpy(capabilityList[fileIndex].filename, filename);
    strcpy(capabilityList[fileIndex].owner, files[fileIndex].owner);
    strcpy(capabilityList[fileIndex].group, group);
    strcpy(capabilityList[fileIndex].permissions, newPermissions);
    printf("***CAPABILITY LISTS*****\n");
    printf("filename: %s\n", capabilityList[fileIndex].filename);
    printf("owner: %s\n",  capabilityList[fileIndex].owner);
    printf("group: %s\n", capabilityList[fileIndex].group);
    printf("permissions: %s\n", capabilityList[fileIndex].permissions);
    printf("%s成功修改檔案 %s 的權限\n", username, filename);

}

void processCommand(int client_socket, const char *command) {
    // 印出接收到的資料
    printf("Received data from client (%d): %s\n", client_socket, command);

    // 這裡假設最多有三個 token，你可以根據實際需求調整
    char order[MAX_BUFFER_SIZE];
    char group[MAX_BUFFER_SIZE];
    char member[MAX_BUFFER_SIZE];

    // 將 command 複製到一個可修改的字串，因 strtok 會修改原始字串
    char commandCopy[MAX_BUFFER_SIZE];
    strncpy(commandCopy, command, sizeof(commandCopy));

    // 使用 strtok 分割字串
    char *token = strtok(commandCopy, ",");
    
    // Process and print each part of the command
    for (int i = 1; token != NULL && i <= 3; ++i) {
        // 儲存 token 到相應的變數
        switch (i) {
            case 1:
                strncpy(order, token, sizeof(order));
                break;
            case 2:
                strncpy(group, token, sizeof(group));
                break;
            case 3:
                strncpy(member, token, sizeof(member));
                break;
        }

        // Get the next token
        token = strtok(NULL, ",");
    }

    // 在這裡添加你的特定命令處理邏輯，使用 order、group、member
    // 將 order 變數透過空格切分成多個 token，並存成多個變數
    char *order_tokens[MAX_BUFFER_SIZE];
    int order_token_count = 0;

    // 將 order 複製到一個可修改的字串，因 strtok 會修改原始字串
    char orderCopy[MAX_BUFFER_SIZE];
    strncpy(orderCopy, order, sizeof(orderCopy));

    // 使用 strtok 分割 order
    token = strtok(orderCopy, " ");
    while (token != NULL && order_token_count < MAX_BUFFER_SIZE) {
        order_tokens[order_token_count++] = token;
        token = strtok(NULL, " ");
    }

    if (strcmp(order_tokens[0], "create") == 0) {
        createFile(order_tokens[1], member, group, order_tokens[2]);
    } else if (strcmp(order_tokens[0], "read") == 0) {
        readFile(client_socket, order_tokens[1], member, group);
    } else if (strcmp(order_tokens[0], "write") == 0) {
        writeFile(order_tokens[1], member, order_tokens[2], group);
    } else if (strcmp(order_tokens[0], "changemode") == 0){
        changeMode(order_tokens[1], member, group, order_tokens[2]);
    } else{
        printf("無法處理，指令錯誤\n");
    }

}

// 執行緒函數，處理單個客戶端的請求
void *handleClient(void *client_info_ptr) {
    struct ClientInfo *client_info = (struct ClientInfo *)client_info_ptr;

    // 接收來自客戶端的資料
    char buffer[MAX_BUFFER_SIZE] = {0};
    ssize_t bytes_received;

    bytes_received = recv(client_info->client_socket, buffer, MAX_BUFFER_SIZE, 0);

    if (bytes_received > 0) {
        // 處理接收到的數據
        processCommand(client_info->client_socket, buffer);

        // 清空緩衝區
        memset(buffer, 0, sizeof(buffer));

    } else if (bytes_received == 0) {
        printf("Client on port %d disconnected\n", client_info->port);
    } else {
        perror("Receive error");
    }

    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // 建立 socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // 設置 server_address 結構
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // 綁定地址和端口
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Binding error");
        exit(EXIT_FAILURE);
    }

    // 監聽連接
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listening error");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // 接受多個連接
    while (1) {
        // 接受連接
        int client_socket;
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) == -1) {
            perror("Acceptance error");
            exit(EXIT_FAILURE);
        }

        // 獲取連接的端口
        int client_port = ntohs(client_address.sin_port);

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), client_port);

        // 將 client_socket 和對應的 port 傳遞給執行緒
        struct ClientInfo *client_info = malloc(sizeof(struct ClientInfo));
        client_info->client_socket = client_socket;
        client_info->port = client_port;

        // 使用執行緒處理客戶端請求
        pthread_t tid;
        if (pthread_create(&tid, NULL, handleClient, client_info) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }

        // 在執行緒中處理下一個客戶端的連接
        pthread_detach(tid);
    }

    // 關閉伺服器 socket
    close(server_socket);

    return 0;
}
