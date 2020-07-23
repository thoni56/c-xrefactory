;; Override el-gets update functions
(defun el-get-update () (message "You cannot update el-get packages in this sandboxed environment"))
(defun el-get-update-all () (message "You cannot update el-get packages in this sandboxed environment"))
(defun el-get-update-packages-of-type () (message "You cannot update el-get packages in this sandboxed environment"))
