使用glad库与glfw库开发。    
vs包管理器中直接安装glfw库。    
在属性中：C/C++中的常规中添加glad库的附加包含目录：glad/include  
将glad/src中的源码glad.c直接复制到解决方案管理器中的源文件目录下即可。  

VBO（Vertex Buffer Object）—— 顶点缓冲对象  
VBO 是一块 GPU 显存，用于存储顶点数据（如位置、法线、颜色、UV 等）。  
它替代了旧版 OpenGL 的“立即模式”（glBegin/glEnd），实现高效批量传输。  
存放 std::vector<Vertex> 这样的顶点数组。  
通过 glBufferData 将 CPU 内存中的数据上传到 GPU 显存。  

 EBO（Element Buffer Object）—— 索引缓冲对象（也叫 IBO）  
 EBO 是一块 GPU 显存，专门用于存储索引数据（indices）
 它告诉 GPU “用哪些顶点组成三角形”，避免重复顶点。  
 存放 std::vector<unsigned int> indices。  
 配合 glDrawElements 使用，实现索引绘制。  

 VAO（Vertex Array Object）—— 顶点数组对象  
 VAO 是一个“状态容器”，它记录了所有与顶点属性相关的配置。
 它不存储顶点数据本身，而是存储如何解释 VBO/EBO 中的数据。
 VAO 保存以下信息：

哪些 VBO 被绑定到哪些 attribute slot（如 location=0,1...）
每个 attribute 的格式（如 vec3、步长 stride、偏移 offset）
当前绑定的 EBO（索引缓冲）


拉普拉斯算子（Laplacian）
cpp
编辑
float laplacian =
    height[i - 1][j] + height[i + 1][j] +
    height[i][j - 1] + height[i][j + 1] - 4 * height[i][j];
✅ 物理意义：
这是对 二维拉普拉斯算子 ∇²h 的离散近似，表示当前点与其四个邻居的高度差总和。

如果周围比中心高 → laplacian > 0 → 中心会被“推高”
如果周围比中心低 → laplacian < 0 → 中心会“下陷”
📌 这是波动传播的核心驱动力！


