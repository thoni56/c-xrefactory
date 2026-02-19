/* Symbol only exists if -DFEATURE_ENABLED is in .c-xrefrc */
#ifdef FEATURE_ENABLED
int feature_function(void) {
    return 42;
}
#endif

int main(void) {
    return feature_function();
}
