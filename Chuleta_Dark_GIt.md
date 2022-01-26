# **Chuleta de control de versiones con git**

## 1. Conceptos mínimos que tenemos que saber sobre el control de versiones





## 2. Que tenemos que saber hacer con Git (y GitHub)






# 2.1. Interprete de comando de git-bash



# 2.2. Control de versiones centralizado

Comando        | Argumentos                                      | Función 
---------------|-------------------------------------------------|------------
**git config** | [--global][--local] http.proxy &lt;domain:port> | Configura distintos aspectos de git, en todo el PC o en el repositorio actual. Por ejemplo, el servidor proxy.
**git clone**  | &lt;URL>                                        | Realiza una copia de un repositorio remoto en un directorio local.
**git remote** | add &lt;URL>                                    | Enlaza nuestro repositorio local con un repositorio remoto, donde se subirán los cambios al realizar push.
**git pull**   | &lt;remote> &lt;branch>                         | Descarga los archivos actualizados del repositorio remoto y realiza un merge entre el repositorio local y el remoto.
**git merge**  | &lt;branch>                                     | Incorpora los cambios  realizados en los commits de otra rama al repositorio local.
**git push**   | &lt;remote> &lt;branch>                         | Actualiza el repositorio remoto con los cambios realizados en los commits locales que se han realizado desde el último push.

# 2.3. Control de versiones distribuido

Comando              | Argumentos                   | Función 
---------------------|------------------------------|------------
**git branch**       | &lt;branch_name>             | Crea una rama nueva en el repositorio local.
**git checkout**     | &lt;branch_name>             | Cambia de rama en el repositorio local.
**git push**         | [-u] &lt;remote> &lt;branch> | Envía la rama al repositorio remoto.

Operaciones de Github:

Operacion                                 | Descripción
------------------------------------------|------------
**pull request** (entre ramas)            | En la página principal del repositorio: "New pull request" y se selecciona la rama deseada
**pull request** (entre repositorios)     | Dentro de la misma opción anterior, seleccionando: "compare across forks"