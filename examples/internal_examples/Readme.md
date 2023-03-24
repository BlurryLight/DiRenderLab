这个文件夹的例子是用来做个人测试用的。

Programs in this directory is for private tests.


# Dynamic Index

用来测试bindless的功能。
`layout(bindless_sampler) uniform sampler2D  diffuseMaps[128];`

依靠从越的文章来分类，OpenGl的Bindless扩展大概只能到 **有限bindless** 的程度，没法到Dx12那样，可以在Shader里声明unbounded srv array。
> 参考：[游戏引擎随笔 0x13：现代图形 API 的 Bindless - 知乎](https://zhuanlan.zhihu.com/p/136449475)

![Readme-2023-03-24-16-49-31](https://img.blurredcode.com/img/Readme-2023-03-24-16-49-31.png?x-oss-process=style/compress)