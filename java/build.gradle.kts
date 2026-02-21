plugins {
    java
    application
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(18)
    }
}

application {
    mainClass = "com.hindu.calendar.cli.HinduCalendarCli"
}

repositories {
    mavenCentral()
}

dependencies {
    testImplementation("org.junit.jupiter:junit-jupiter:5.9.3")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.test {
    useJUnitPlatform()
    jvmArgs("-Xmx512m")
}

tasks.withType<JavaCompile> {
    options.encoding = "UTF-8"
}
