UTF-8

环境：
操作系统 linux
boost库 1.70

使用方法：
1、make：编译生成buildIndex和searchQuery两个可执行文件置于该目录下。
2、./buildIndex your_doc_input_directory：对your_doc_input_directory目录下文档集进行索引构建，结果会生成index目录存放倒排索引文件和文档对应id文件。
3、./searchQuery：执行searchQuery进行查询，每个查询由多个词组组成，以空格或tab隔开。可不断输入查询。输入/q退出程序。
4、make clean：清除index文件夹、buildIndex和searchQuery。
