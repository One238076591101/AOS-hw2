# AOS-hw2
System Protection &amp; Distributed Synchronization 
Part1：建立一台server來管理為clients提供的文件

請注意，所有客戶端操作的檔案都位於同一目錄中伺服器端。 伺服器必須使用CAPABILITY LISTS來管理權限文件。 您必須向助教展示每個操作的能力清單如何變化伺服器端。
客戶端：group members分為AOS-members,CSE-members兩種group，AOS-members共3人，分別是anne,ben,claire ，CSE-members共3人，分別是deenis,eric,fred
檔案存取權限：有 read, write兩種
存取權限者分為 file owner, group members, and other people(different group members)三種

	• 當檔案被允許read時（或write），客戶端能夠下載（或上傳）檔案。
	• 如果客戶端在未經許可的情況下請求對文件進行操作，伺服器應該禁止它並列印一條訊息來說明原因。
	• 每個客戶端都能夠動態建立文件但應指定所有存取權限。 例如，Ken 可以執行以下指令命令：
	1) create homework2.txt rwrw--  create homework5.c rwrw-w  create example.txt rwrwrw
	第一個指令「create」是幫助Ken在伺服器上建立一個文件，其中第三個參數給予文件的權限（「r」和「w」分別代表讀取和寫入權限，“-”表示無權限）
	rwr---意思為file owner有讀取和寫入權限，group members只有讀取沒有寫入權限，other people(different group members)則都沒有讀取和寫入權限
	2) read homework2.c  read homework2.txt
	第二個命令“read ”，例如允許客戶端從伺服器下載檔案(僅當他有相應的權限並且檔案確實存在)
	3) write homework2.c o/a   write homework2.txt a
	第三個指令「write」允許客戶端上傳（並修改）現有文件，其中第三個參數可以是“o”或“a”，o代表overwrite，允許客戶端覆蓋原始文件，a代表append，可以將其數據附加到文件末尾。 前提為客戶端有相應的權限，就可以寫入該文件。
	並且該文件確實存在。
	4) changemode homework2.c rw---- 
	最後一個指令「changemode」用來修改檔案的讀取和寫入權限。 修改後的權限對以下操作生效改變命令。 

Part2：
	
1.當一個客戶端正在寫入檔案時，其他客戶端無法同時讀取或寫入相同檔案，若有上述行為發生，伺服器必須跳出警告訊息阻擋行為。 
2.另外，當一個客戶端正在讀取檔案時，其他客戶端無法寫入該檔案，若有上述行為發生，伺服器必須跳出警告訊息阻擋行為。 
3.伺服器必須能夠連接多個客戶端並允許多個客戶端「同時」讀取檔案。

因此，在本作業的第二部分中，您將被要求在伺服器-客戶端架構中套用上述規則。並且要求在 UNIX 相容系統中使用標準 C socket library。socket會連接上傳(write)或下載(read)檔案，檔案大小應該要足夠大以顯示上述行為。
![image](https://github.com/One238076591101/AOS-hw2/assets/59820619/6b30e4a1-dab2-4fad-82de-785db219d67a)
