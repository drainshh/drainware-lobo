@echo off
rem Удаление кеша проекта
rmdir /s /q .vs
rmdir /s /q intermediates
rmdir /s /q debug
rmdir /s /q release
echo Кеш проекта удален.