# MiniSQL
## Include
## API
## BufferManager
### 数据文件定义
每个文件块大小默认为4096字节  
本数据库仅支持定长记录，数据库系统的文件管理基于此前提设计    
三类文件：记录数据文件、索引数据文件和Catalog数据文件  
记录数据文件：  
* 每张表单独定义一个记录数据文件，采用堆文件存储
* 文件首部第一个块保留，存储文件中的最后一个块的块地址，每个记录的字节数，块间空余指针的头结点（全0表示NULL）；余下的所有块均为数据块
* 每个数据块首部保留6字节，前4字节表示块间空余指针，后2字节表示块内空余指针;每个记录的首部附加1个字节，标识当前块是否被使用（1表示使用），若当前块为空，则之后的2个字节为标识块内下一个空间块的首地址（全0表示NULL）

索引数据文件：  
* 每张表单独定义一个索引数据文件  
* 文件首部第一个块保留，存储文件中的最后一个块的块地址，索引B+数的根节点块地址，下一个空余块地址的指针；余下的所有块均为节点块
* 每个节点块的内容由IndexManager决定，节点不应存储文件地址，所需信息可运行时通过接口获取

Catalog数据文件：  
* 整个数据库共用一个文件
* 文件地址固定为0，文件名固定为Catalog，文件内容由CatalogManager决定

### 文件虚拟地址定义
地址共64位，按高位至低位划分为16位文件寻址，32位块寻址，16位块偏移地址  

### 接口定义
允许用户输入文件名与文件类型打开文件，文件将自动获取当前最大文件地址+1的文件地址  
允许用户利用2字节地址关闭并删除相应的文件  
允许用户利用2字节地址获取相应文件的下一个空余记录的地址，当记录地址返回后，该空余记录默认已被占用  
允许用户利用2字节地址从索引文件获取下一个空余块的地址  
允许用户利用2字节地址从索引文件获取根节点块的地址  
允许用户利用6字节地址获取相应块的首地址  
允许用户利用6字节地址将相应块固定在缓冲区中直到用户手动解除固定  
允许用户利用8字节地址得到相应缓冲区偏移位置的内存地址
允许用户利用8字节地址删除以该地址为首的记录且将其标记为空余

## CatalogManager
## IndexManager
## Interpreter
## RecordManager