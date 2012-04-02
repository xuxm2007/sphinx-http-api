set helplang=cn
set winaltkeys=no "Alt组合键不映射到菜单上
if v:lang =~ "utf8$" || v:lang =~ "UTF-8$"
  set fileencodings=utf-8,latin1
endif

syntax on
filetype on
filetype indent on
filetype plugin on
"colorscheme ron
"set background=dark

hi Error80        gui=NONE   guifg=#ffffff   guibg=#6e2e2e
hi ErrorLeadSpace gui=NONE   guifg=#ffffff   guibg=#6e2e2e
hi ErrorTailSpace gui=NONE   guifg=#ffffff   guibg=#6e2e2e

syn keyword cType int32_t int64_t
syn keyword cOperator likely
syn match ErrorLeadSpace /^ \+/
syn match ErrorTailSpace / \+$/
syn match Error80 /\%>80v.\+/

if has('autocmd')
  au BufReadPost * if line("'\"") > 1 && line("'\"") <= line("$") | exe "normal! g'\"" | endif
  filetype plugin indent on
endif

set nocompatible
set bs=indent,eol,start
set viminfo='20,\"50
set ignorecase
set smartcase
set autowrite
set autoindent
set tabstop=2
set softtabstop=2
set shiftwidth=2
set expandtab
set cindent
set cinoptions={0,1s,t0,n-2,p2s,(03s,=.5s,>1s,=1s,:1s
set showmatch
set linebreak
set whichwrap=b,s,<,>,[,]
set mouse=a
set number
set previewwindow
set history=50
set laststatus=2
set ruler
set showcmd
set showmode
set incsearch
set hlsearch
set textwidth=78
set cc=81
set nocp

"--ctags setting--
"" 按下F5重新生成tag文件，并更新taglist
map <F5> :!ctags -R --c++-kinds=+p --fields=+iIaS --extra=+q .<CR><CR> :TlistUpdate<CR>
imap <F5> <ESC>:!ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .<CR><CR> :TlistUpdate<CR>
set tags=tags
set tags+=./tags
set tags+=~/arm/linux-2.6.24.7/tags 

"-- omnicppcomplete setting --
"按下F3自动补全代码
map <F3> <C-X><C-O>
"按下F2根据头文件内关键字补全
imap <F2> <C-X><C-I>
set completeopt=menu,menuone " 关掉智能补全时的预览窗口
let OmniCpp_MayCompleteDot = 1 " autocomplete with .
let OmniCpp_MayCompleteArrow = 1 " autocomplete with ->
let OmniCpp_MayCompleteScope = 1 " autocomplete with ::
let OmniCpp_SelectFirstItem = 2 " select first item (but don't insert)
let OmniCpp_NamespaceSearch = 2 " search namespaces in this and included files
let OmniCpp_ShowPrototypeInAbbr = 1 " show function prototype in popup window
let OmniCpp_GlobalScopeSearch=1 " enable the global scope search
let OmniCpp_DisplayMode=1 " Class scope completion mode: always show all members
"let OmniCpp_DefaultNamespaces=["std"]
let OmniCpp_ShowScopeInAbbr=1 " show scope in abbreviation and remove the last column
let OmniCpp_ShowAccess=1

"-- Taglist setting --
let Tlist_Ctags_Cmd='ctags' "因为我们放在环境变量里，所以可以直接执行
let Tlist_Use_Right_Window=0 "让窗口显示在右边，0的话就是显示在左边
let Tlist_Show_One_File=0 "让taglist可以同时展示多个文件的函数列表
let Tlist_File_Fold_Auto_Close=1 "非当前文件，函数列表折叠隐藏
let Tlist_Exit_OnlyWindow=1 "当taglist是最后一个分割窗口时，自动推出vim
"是否一直处理tags.1:处理;0:不处理
let Tlist_Process_File_Always=1 "实时更新tags
let Tlist_Inc_Winwidth=0

"-- WinManager setting --
let g:winManagerWindowLayout='FileExplorer|TagList' " 设置我们要管理的插件
"let g:persistentBehaviour=0 " 如果所有编辑文件都关闭了，退出vim
nmap wm :WMToggle<cr>

" -- MiniBufferExplorer -- 
let g:miniBufExplMapWindowNavVim = 1 "按下Ctrl+h/j/k/l，可以切换到当前窗口的上下左右窗口
let g:miniBufExplMapWindowNavArrows = 1 "按下Ctrl+箭头，可以切换到当前窗口的上下左右窗口
let g:miniBufExplMapCTabSwitchBufs = 1 "启用以下两个功能：Ctrl+tab移到下一个buffer并在当前窗口打开；Ctrl+Shift+tab移到上一个buffer并在当前窗口打开；ubuntu好像不支持
"let g:miniBufExplMapCTabSwitchWindows = 1 "启用以下两个功能：Ctrl+tab移到下一个窗口；Ctrl+Shift+tab移到上一个窗口；ubuntu好像不支持
let g:miniBufExplModSelTarget = 1    "不要在不可编辑内容的窗口（如TagList窗口）中打开选中的buffer

"--fold setting--
set foldmethod=syntax " 用语法高亮来定义折叠
set foldlevel=100 " 启动vim时不要自动折叠代码
set foldcolumn=5 " 设置折叠栏宽度

"-- QuickFix setting --
" 按下F6，执行make clean
map <F6> :make clean<CR><CR><CR>
" 按下F7，执行make编译程序，并打开quickfix窗口，显示编译信息
map <F7> :make<CR><CR><CR> :copen<CR><CR>
" 按下F8，光标移到上一个错误所在的行
map <F8> :cp<CR>
" 按下F9，光标移到下一个错误所在的行
map <F9> :cn<CR>
" 以上的映射是使上面的快捷键在插入模式下也能用
imap <F6> <ESC>:make clean<CR><CR><CR>
imap <F7> <ESC>:make<CR><CR><CR> :copen<CR><CR>
imap <F8> <ESC>:cp<CR>
imap <F9> <ESC>:cn<CR>
